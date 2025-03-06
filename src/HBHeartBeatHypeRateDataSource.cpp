#include "HeartBeatDataSource.hpp"
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <jni.h>
#include <memory>
#include <mutex>
#include <stdlib.h>
#include <string>
#include <sys/endian.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include <system_error>
#include <unistd.h>
#include "BeatLeaderRecorder.hpp"
#include "ModConfig.hpp"
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/document.h"
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/stringbuffer.h"
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/writer.h"
#include "i18n.hpp"
#include "main.hpp"

#include <websocketpp/config/asio_client.hpp>

#include <websocketpp/client.hpp>
#include <websocketpp/connection.hpp>
#include <websocketpp/frame.hpp>

/*

You know you won't copy these code to get heart rate in other project
because it uses a private server.
If you have similar needs, please contact HypeRate official, they are kind people. heart. :) 

*/
namespace HeartBeat{

DECLARE_DATA_SOURCE(HeartBeatHypeRateDataSource)

typedef websocketpp::client<websocketpp::config::asio_client> client;

client endpoint;

HeartBeatHypeRateDataSource::HeartBeatHypeRateDataSource(){
    Recorder::heartDeviceName = HEART_DEV_NAME_HYPERATE;
    this->CreateSocket();
}

client::connection_ptr con = nullptr;
client::timer_ptr the_timer = nullptr;
time_t last_ping_time = 0;
bool con_opened = false;


int failed_count = 0;

int retry_sleep_time(){
    if(failed_count < 3){
        return (1);
    }else if(failed_count < 10){
        return (10);
    }else{
        return (15);
    }
}

std::string CheckHypeRateWebSocketIdentity(){
    std::string ret = getModConfig().HypeRateWebSocketIdentity.GetValue();
    if(ret == ""){
        char buff[33];
        FILE * f = fopen("/dev/urandom", "rb");
        bool handled = false;
        const char * avaliable_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()";
        int char_len = strlen(avaliable_chars);
        if(f){
            getLogger().info("HypeRate Websocket random identity generrated from /dev/urandom");
            uint8_t numbers[32];
            if(32 == fread(numbers, 1, 32, f)){
                handled = true;
                for(int i=0;i<32;i++){
                    buff[i] = avaliable_chars[numbers[i] % char_len];
                }
                buff[32] = '\0';
            }
        }

        if(f){
            fclose(f);
            f = NULL;
        }

        if(!handled){
            getLogger().warn("HypeRate Websocket random identity not generated, fallback to random call");
            for(int i=0;i<32;i++){
                buff[i] = avaliable_chars[random() % char_len];
            }
            buff[32] = '\0';
        }

        getModConfig().HypeRateWebSocketIdentity.SetValue(buff);
        ret = buff;
    }
    return ret;
}

std::function<void(std::error_code)> timer_impl;
int current_retry_time_already = 0;
void HeartBeatHypeRateDataSource::CreateSocket(){

    endpoint.set_access_channels(websocketpp::log::alevel::all);
    endpoint.set_error_channels(websocketpp::log::elevel::all);

    // Initialize ASIO
    endpoint.init_asio();

    timer_impl = [this](std::error_code e){
        the_timer = endpoint.set_timer(200, timer_impl);

        current_retry_time_already += 200;

        if(resetRequest){
            resetRequest = false;
            if(con && con->get_state() != websocketpp::session::state::closed)
                con->close(1000, "reset requested");
            con = nullptr;
            failed_count = 0;
            return;
        }


        if(current_retry_time_already <= retry_sleep_time() * 1000){
            return;
        }

        if(closed){
            if(the_timer)
                the_timer->cancel(), the_timer = nullptr;
            if(con)
                con->close(1001, "closed"), con = nullptr;
            return;
        }

        if(con == nullptr){
            if(needConnection && getModConfig().HypeRateId.GetValue().length() > 0){
                websocketpp::lib::error_code ec;
                con = endpoint.get_connection("ws://heart.0xf7.top/ws", ec);
                con_opened = false;
                if(ec){
                    getLogger().error("HypeRate connection error: {}", ec.message());
                    con = nullptr;
                    failed_count++;
                    return;
                }else{
                    endpoint.connect(con);
                    getLogger().info("heart server has been connected");
                    return;
                }
            }else{
                    return;
            }
        }

        if(con){
            time_t now = time(NULL);
            if(con_opened && now > last_ping_time + 3 && con->get_state() == websocketpp::session::state::open){
                last_ping_time = now;
                getLogger().info("ping");
                auto error = con->send("i");
                if(error){
                    failed_count++;
                    //error
                    con->close(1002, "error");
                    con = nullptr;
                };
            }
        }

    };

    the_timer = endpoint.set_timer(200, timer_impl);

    // Register our handlers
    endpoint.set_socket_init_handler([](std::weak_ptr<void> a,
        boost::asio::basic_stream_socket<boost::asio::ip::tcp> &b){
        
    });
    // endpoint.set_tls_init_handler();
    endpoint.set_message_handler([this](std::weak_ptr<void> a, 
        std::shared_ptr<websocketpp::message_buffer::message<
        websocketpp::message_buffer::alloc::con_msg_manager>> b){
        if(b->get_opcode() != websocketpp::frame::opcode::text ){
            //we can only handle text opcode
            return;
        }
        failed_count = 0;
        auto & payload = b->get_payload();
        if(payload == "o"){
            //this is a ping command
            getLogger().info("pong");
            return;
        }
        try{
            getLogger().info("{}", payload);
            if(payload.length() > 1 && payload[0] == 'S'){
                const char * json_str = payload.c_str() + 1;
                rapidjson::Document d;
                d.Parse(json_str);
                if(!d.IsObject())
                    return;
                auto type_it = d.FindMember("type");
                if(type_it == d.MemberEnd())
                    return;
                if(!type_it->value.IsString())
                    return;
                std::string type = type_it->value.GetString();
                if(type == "message"){
                    auto msg_it = d.FindMember("msg");
                    auto actions_it = d.FindMember("actions");
                    std::vector<std::string> actions;

                    if(msg_it != d.MemberEnd() && msg_it->value.IsString()){
                        size_t len = msg_it->value.GetStringLength();
                        std::lock_guard<std::mutex> g(this->message_from_server_mutex);


                        if(len + 10 >= sizeof(this->message_from_server)){
                            size_t copy_len = sizeof(this->message_from_server) - 10;

                            memcpy(this->message_from_server, msg_it->value.GetString(), copy_len);
                            this->message_from_server[copy_len] = '.';
                            this->message_from_server[copy_len+1] = '.';
                            this->message_from_server[copy_len+2] = '.';
                            this->message_from_server[copy_len+3] = '\0';
                        }else{
                            memcpy(this->message_from_server, msg_it->value.GetString(), len);
                            this->message_from_server[len] = '\0';
                        }
                        this->has_message_from_server = true;
                    }else{
                        std::lock_guard<std::mutex> g(this->message_from_server_mutex);
                        strcpy(this->message_from_server, "invalid server message");
                    }

                    if(actions_it != d.MemberEnd() && actions_it->value.IsArray()){
                        for(auto & e : actions_it->value.GetArray()){
                            if(e.IsString()){
                                const char * action = e.GetString();
                                //do the action here
                                if(strcmp(action, "close") == 0){
                                    closed = true;
                                    con->close(1001, "server close, never open");
                                    con = nullptr;
                                }

                                if(strcmp(action, "reset") == 0){
                                    getModConfig().HypeRateWebSocketIdentity.SetValue("");
                                }
                            }
                        }
                    }
                }
            }

            {
                const char * json_str = payload.c_str();
                rapidjson::Document d;
                d.Parse(json_str);
                if(!d.IsObject())
                    return;

                auto it = d.FindMember("payload");
                if(it == d.MemberEnd())
                    return;
                auto & payload = it->value;

                if(!payload.IsObject())
                    return;
                auto hr_it = payload.FindMember("hr");
                if(hr_it == payload.MemberEnd())
                    return;
                if(!hr_it->value.IsInt())
                    return;
                int heart = hr_it->value.GetInt();
                std::atomic_thread_fence(std::memory_order_acquire);
                this->the_heart = heart;
                std::atomic_thread_fence(std::memory_order_acquire);
                this->has_unread_heart_data = true;
                std::atomic_thread_fence(std::memory_order_acquire);
            }
    
    
        }catch(...){

        }
        //TODO: json load payload
    });
    endpoint.set_open_handler([](std::weak_ptr<void> a){
        getLogger().info("connection open_handler executed");
        std::string id = getModConfig().HypeRateId.GetValue();
        // id = "internal-testing";
        rapidjson::Document dom;
        dom.SetObject();
        dom.AddMember("_id", CheckHypeRateWebSocketIdentity(), dom.GetAllocator());
        dom.AddMember("id", id, dom.GetAllocator());
        dom.AddMember("lang", rapidjson::StringRef(LANG->lang_name), dom.GetAllocator());
        dom.AddMember("ver", VERSION, dom.GetAllocator());

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        dom.Accept(writer);

        std::string toSend = std::string("C") + buffer.GetString();
        getLogger().info("Send package to server: {}", toSend);
        if(con->get_state() == websocketpp::session::state::open && con->send(toSend)){
            getLogger().error("connection send failed.");
            con->close(1000, "error");
            con = nullptr;
        }else{
            con_opened = true;
            failed_count = 0;
        }
    });
    endpoint.set_close_handler([](std::weak_ptr<void> b){
        getLogger().info("the connection has been closed");
        con = nullptr;
        failed_count++;
    });
    endpoint.set_fail_handler([](auto f){
        if(con && con->get_state() == websocketpp::session::state::open) con->close(1007, "failed"),con = nullptr;
        failed_count++;
    });


    pthread_t the_thread;
    pthread_create(&the_thread, NULL, HeartBeatHypeRateDataSource::ServerThread, this);
}


void * HeartBeatHypeRateDataSource::ServerThread(void *self){
    HeartBeatHypeRateDataSource * me = (decltype(me))self;

    auto retry = [](){
        sleep(3);
        try{
            con->close(1004,"cpp exception");
        }catch(...){
            con = nullptr;
        }
    };
    while(!me->closed){
        try{
            endpoint.run();
            timer_impl(std::error_code());
        } catch (websocketpp::exception const & e) {
            getLogger().error("websocketpp exception {}", e.what());
            retry();
        } catch (std::exception const & e) {
            getLogger().error("std exception exception {}", e.what());
            retry();
        } catch (...) {
            getLogger().error("other exception");
            retry();
        }
    }
    return nullptr;
}

bool HeartBeatHypeRateDataSource::GetData(int&heartbeat){
    if(has_unread_heart_data)
    {
        has_unread_heart_data = false;
        heartbeat = the_heart;
        return true;
    }
    return false;
}

}