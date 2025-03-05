#include "HeartBeatDataSource.hpp"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <jni.h>
#include <memory>
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
#include "main.hpp"

#include <websocketpp/config/asio_client.hpp>

#include <websocketpp/client.hpp>
#include <websocketpp/frame.hpp>

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


void HeartBeatHypeRateDataSource::CreateSocket(){

    endpoint.set_access_channels(websocketpp::log::alevel::all);
    endpoint.set_error_channels(websocketpp::log::elevel::all);

    // Initialize ASIO
    endpoint.init_asio();

    the_timer = endpoint.set_timer(200, [this](std::error_code e){
        if(closed){
            if(the_timer)
                the_timer->cancel(), the_timer = nullptr;
            if(con)
                con->close(1001, "closed"), con = nullptr;
            return;
        }

        if(con == nullptr){
            if(needConnection){
                websocketpp::lib::error_code ec;
                con = endpoint.get_connection("ws://heart.0xf7.top/ws", ec);
                if(ec){
                    getLogger().error("HypeRate connection error: {}", ec.message());
                    con = nullptr;
                    sleep(10);
                    return;
                }else{
                    getLogger().info("heart server has been connected");
                }
        }else{
                return;
            }
        }

        if(con){
            if(con->send("i")){
                //error
                con->close(1002, "error");
                con = nullptr;
            };
        }
    });

    // Register our handlers
    endpoint.set_socket_init_handler([](std::weak_ptr<void> a,
        boost::asio::basic_stream_socket<boost::asio::ip::tcp> &b){
        
    });
    // endpoint.set_tls_init_handler();
    endpoint.set_message_handler([](std::weak_ptr<void> a, 
        std::shared_ptr<websocketpp::message_buffer::message<
        websocketpp::message_buffer::alloc::con_msg_manager>> b){
        if(b->get_opcode() != websocketpp::frame::opcode::text ){
            //we can only handle text opcode
            return;
        }
        auto & payload = b->get_payload();
        if(payload == "o"){
            //this is a ping command
            return;
        }

        //TODO: json load payload
    });
    endpoint.set_open_handler([](std::weak_ptr<void> a){
        //TODO: send connect json
    });
    endpoint.set_close_handler([](std::weak_ptr<void> b){
        con = nullptr;
    });
    endpoint.set_fail_handler([](auto f){
        if(con) con->close(1007, "failed"),con = nullptr;
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