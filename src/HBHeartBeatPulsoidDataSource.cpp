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

namespace HeartBeat{

DECLARE_DATA_SOURCE(HeartBeatPulsoidDataSource)

typedef websocketpp::client<websocketpp::config::asio_client> client;

static client endpoint;

HeartBeatPulsoidDataSource::HeartBeatPulsoidDataSource(){
    Recorder::heartDeviceName = HEART_DEV_NAME_PULSOID;
    this->CreateSocket();
}

static client::connection_ptr con = nullptr;
static client::timer_ptr the_timer = nullptr;
static time_t last_ping_time = 0;
static bool con_opened = false;


static int failed_count = 0;

inline int retry_sleep_time(){
    if(failed_count < 30)
        return 3;
    return 10;
}

static std::function<void(std::error_code)> timer_impl;
static int current_retry_time_already = 0;
void HeartBeatPulsoidDataSource::CreateSocket(){

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
            failed_count = 0;
            return;
        }


        if(current_retry_time_already <= retry_sleep_time() * 1000){
            return;
        }
        current_retry_time_already = 0;

        if(closed){
            if(the_timer)
                the_timer->cancel(), the_timer = nullptr;
            if(con)
                con->close(1000, "closed");
            return;
        }

        if(con && con->get_state() == websocketpp::session::state::closed){
            con = nullptr;
            failed_count++;
        }

        if(con == nullptr){
            if(displayWanted && getModConfig().PulsoidToken.GetValue() != "00000000-0000-0000-0000-000000000000"){
                websocketpp::lib::error_code ec;
                std::string url = "ws://dev.pulsoid.net/api/v1/data/real_time?response_mode=text_plain_only_heart_rate&access_token=" + getModConfig().PulsoidToken.GetValue();
                con = endpoint.get_connection(url, ec);
                con_opened = false;
                if(ec){
                    getLogger().error("Pulsoid connection error: {}", ec.message());
                    failed_count++;
                    return;
                }else{
                    con->set_open_handshake_timeout(5000);
                    con->set_close_handshake_timeout(1000);
                    con->set_pong_timeout(3000);
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
            if(con_opened && now > last_ping_time + 5 && con->get_state() == websocketpp::session::state::open){
                last_ping_time = now;
                // getLogger().info("ping");
                con->ping("");
            }
        }

    };

    the_timer = endpoint.set_timer(200, timer_impl);

    // Register our handlers
    endpoint.set_socket_init_handler([](std::weak_ptr<void> a,
        boost::asio::basic_stream_socket<boost::asio::ip::tcp> &b){
        
    });
    endpoint.set_ping_handler([](auto r, auto m){
        return true;
    });
    endpoint.set_pong_handler([](auto r, auto p){
        failed_count = 0;
    });
    endpoint.set_pong_timeout_handler([](auto r, auto p){
        if(con && con->get_state() == websocketpp::session::state::open){
            con->close(1000, "pong timeout");
        }
        getLogger().warn("Network ping-pong timeout");
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
        the_heart = atoi(payload.c_str());
        has_unread_heart_data = true;
    });
    // endpoint.set_open_handler([](std::weak_ptr<void> a){
    //     getLogger().info("connection open_handler executed");
    // });
    endpoint.set_close_handler([](std::weak_ptr<void> b){
        getLogger().info("the connection has been closed");
        con = nullptr;
    });
    
    endpoint.set_fail_handler([](auto f){
        getLogger().info("connection failed, retry later");
        if(con && con->get_state() == websocketpp::session::state::open) con->close(1000, "failed");
        failed_count++;
        con = nullptr;
    });


    pthread_t the_thread;
    pthread_create(&the_thread, NULL, HeartBeatPulsoidDataSource::ServerThread, this);
}


void * HeartBeatPulsoidDataSource::ServerThread(void *self){
    HeartBeatPulsoidDataSource * me = (decltype(me))self;

    auto retry = [](){
        sleep(3);
        try{
            if(con && con->get_state() == websocketpp::session::state::open)con->close(1000,"cpp exception");
        }catch(...){
            // con = nullptr;
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

bool HeartBeatPulsoidDataSource::GetData(int&heartbeat){
    if(has_unread_heart_data)
    {
        has_unread_heart_data = false;
        heartbeat = the_heart;
        return true;
    }
    return false;
}

}
