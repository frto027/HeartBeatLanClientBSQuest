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

#define ASIO_STANDALONE
#include <websocketpp/config/asio_no_tls_client.hpp>

#include <websocketpp/client.hpp>
#include <websocketpp/connection.hpp>
#include <websocketpp/frame.hpp>

#include <curl/curl.h>

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
    if(failed_count < 20)
        return 6;
    return 12;
}

static std::function<void(std::error_code)> timer_impl;
static int current_retry_time_already = 0;

size_t curl_write_callback(char *ptr, size_t size, size_t nmemb, std::string * str){
    if(str)
        str->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

void HeartBeatPulsoidDataSource::CreateSocket(){

    endpoint.set_access_channels(websocketpp::log::alevel::all);
    endpoint.set_error_channels(websocketpp::log::elevel::all);
    {
        char buff[1024] = "";
        sprintf(buff, "%s %s", "HeartBeatQuest/" VERSION " BeatSaber/" GAME_VERSION, getQuestDeviceName());
        endpoint.set_user_agent(buff);
    }

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
            if(safe_pair_wanted){
                    getLogger().info("Start safe pair");
                    safe_pairing = true;
                    safe_pair_wanted = false;
                    safe_pair_done_wanted = false;

                    std::string pair_token, header_string;
                    auto ua = "HBQ/" VERSION " BS/" GAME_VERSION " " + std::string(LANG->lang_name) + " " + CheckHypeRateWebSocketIdentity();

                    //get pair token
                    {
                        auto curl = curl_easy_init();
                        auto url = std::string(SERVER_HOST "/pulsoid/safe/start");
                        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
                        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
                        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
                        curl_easy_setopt(curl, CURLOPT_USERAGENT, ua.c_str());
                        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
                        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &pair_token);
                        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_string);
                        curl_easy_perform(curl);
                        curl_easy_cleanup(curl);
                    }
                    // getLogger().debug("Pair token is {}", pair_token);

                    if(pair_token.size() > 0 && pair_token.size() < 80){
                        if(pair_token[0] == '?'){
                            safe_pairing = false;
                            safe_pair_wanted = false;
                            err(pair_token.c_str() + 1);
                            return;
                        }
                        err("");
                        //continue
                        // auto login_url = SERVER_HOST "/pulsoid/safe/redir?token=" + pair_token;
                        auto login_url = SERVER_HOST "/pulsoid/safe/redir?token=" + pair_token;

                        {
                            //we will open the url in the setthings thread
                            std::lock_guard<std::mutex> g(this->url_mutex);
                            this->url = login_url;
                            this->url_open_wanted = true;

                            getLogger().info("open url {}", login_url);
                        }

                        //get token from server
                        auto tokenurl = std::string(SERVER_HOST "/pulsoid/safe/token?token=") + pair_token;

                        
                        for(int i=0;i<15*60;i++){
                            sleep(1);
                            if(safe_pairing == false)
                                break;
                            if(safe_pair_wanted){
                                safe_pairing = false;
                                break;
                            }

                            if(i % 40 == 0){
                                auto keep_alive_url = std::string(SERVER_HOST "/pulsoid/safe/keep_alive?token=") + pair_token;

                                auto curl = curl_easy_init();
                                curl_easy_setopt(curl, CURLOPT_URL, keep_alive_url.c_str());
                                curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
                                curl_easy_setopt(curl, CURLOPT_POST, 1L);
                                curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
                                curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
                                curl_easy_setopt(curl, CURLOPT_USERAGENT, ua.c_str());
                                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
                                curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
                                curl_easy_setopt(curl, CURLOPT_HEADERDATA, NULL);
                                curl_easy_perform(curl);
                                curl_easy_cleanup(curl);
                            }

                            if(safe_pair_done_wanted){
                                safe_pair_done_wanted = false;
                                std::string token, header;
                                auto curl = curl_easy_init();
                                curl_easy_setopt(curl, CURLOPT_URL, tokenurl.c_str());
                                curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
                                curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
                                curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
                                curl_easy_setopt(curl, CURLOPT_USERAGENT, ua.c_str());
                                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
                                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &token);
                                curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header);
                                curl_easy_perform(curl);
                                curl_easy_cleanup(curl);
                                if(token.size() > 0){
                                    if(token[0] == '?'){
                                        if(token == "?authorization_pending")
                                            continue;
                                        getLogger().error("Pair failed: {}", token.c_str());
                                        err(token);
                                        safe_pairing = false;
                                        continue;
                                    }
                                    if(
                                        (token[0] >= 'a' && token[0] <= 'z')
                                        || (token[0] >= 'A' && token[0] <= 'Z')
                                        || (token[0] >= '0' && token[0] <= '9')
                                        || token[0] == '-'
                                    ){
                                        getModConfig().PulsoidToken.SetValue(token);
                                        safe_pairing = false;
                                        modconfig_is_dirty = true;
                                        err("");//succees
                                    }else{
                                        err("Invalid token.");
                                        getLogger().error("Pair failed, invalid token: {}", token.c_str());
                                        safe_pairing = false;
                                    }
                                }else{
                                    safe_pairing = false;
                                }
                            }
                        }
                        if(safe_pairing){
                            err("Retry please. Timeout.");
                        }
                        safe_pairing = false;
                    }else{
                        err("Server error, check your Internet.");
                        safe_pairing = false;
                    }
            }

            if(displayWanted && !safe_pair_wanted && getModConfig().PulsoidToken.GetValue() != "" && getModConfig().PulsoidToken.GetValue() != "00000000-0000-0000-0000-000000000000"){
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
                con->ping("");
            }
            
        }

    };

    the_timer = endpoint.set_timer(200, timer_impl);

    // Register our handlers
    endpoint.set_socket_init_handler([](std::weak_ptr<void> a,
        asio::basic_stream_socket<asio::ip::tcp> &b){
        
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

void HeartBeatPulsoidDataSource::RequestSafePair(){
    safe_pair_wanted = true;
    ResetConnection();
}

}
