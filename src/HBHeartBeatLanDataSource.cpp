#include "main.hpp"
#include "HeartBeatDataSource.hpp"

#include "paper/shared/logger.hpp"

#include <unistd.h>
#include <netinet/in.h>
#include <string.h>

#include "sys/types.h"
#include "sys/socket.h"
#include <poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include <pthread.h>
#include <unistd.h>

#include <algorithm>
#include <BeatLeaderRecorder.hpp>

#define PORT 9965

#define PROTOCOL_VER "001"
#define SERVER_MSG "HeartBeatSenderHere" PROTOCOL_VER
#define CLIENT_MSG "HeartBeatRecHere" PROTOCOL_VER

namespace HeartBeat{

    DECLARE_DATA_SOURCE(HeartBeatLanDataSource)


    void * HeartBeatLanDataSource::ServerThread(void * self){
        while(1){
            ((HeartBeatLanDataSource*)self)->SocketUpdate();
        }
        return NULL;
    }

    bool init_recv_sock(int socket){
        int buf;
        if(-1 == fcntl(socket, F_SETFL, O_NONBLOCK)){
            getLogger().error("recv socket fcntl failed!");
            goto failed;
        }
        buf = 1024;
        if(-1 == setsockopt(socket, SOL_SOCKET, SO_SNDBUF, &buf, sizeof(buf))){
            getLogger().warn("recv socket send buff length set failed!");
        }
        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        if(-1 == bind(socket, (sockaddr*)&addr, sizeof(addr))){
            getLogger().error("recv socket bind failed.");
            goto failed;
        }

        return true;
failed:
        close(socket);
        return false;
    }

    HeartBeatLanDataSource::HeartBeatLanDataSource(){
        Recorder::heartDeviceName = HEART_DEV_NAME_LAN;

        pair_socket = -1;
        recv_socket = -1;

        recv_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if(recv_socket == -1){
            getLogger().error("can't create recv socket");
        }
        if(!init_recv_sock(recv_socket)){
            recv_socket = -1;
        }

        selected_mac = getModConfig().SelectedBleMac.GetValue();


        StartPair();
        if(-1 == pipe2(flush_pipe, O_CLOEXEC|O_DIRECT)){
            getLogger().error("cannot create pipe({})", errno);
            flush_pipe[0] = flush_pipe[1] = -1;
        }


        pthread_t the_thread;
        pthread_create(&the_thread, NULL, HeartBeatLanDataSource::ServerThread, this);
    }

    void HeartBeatLanDataSource::StartPair(){
        if(pair_socket != -1)
            return;
        pair_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if(pair_socket == -1){
            getLogger().warn("Can't create pair socket!");
            return;
        }
        getLogger().info("pair socket created.");
        if(-1 == fcntl(pair_socket, F_SETFL, O_NONBLOCK)){
            getLogger().error("pair socket fcntl failed!");
            goto failed;
        }
        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(PORT);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        if(-1 == bind(pair_socket, (sockaddr*)&addr, sizeof(addr))){
            getLogger().error("pair socket bind failed.");
            goto failed;
        }

        if(flush_pipe[1] != -1){
            write(flush_pipe[1], "\0",1);
        }
        return;

failed:
        close(pair_socket);
        pair_socket = -1;
    }

    
    // this function is executed in other thread
    void HeartBeatLanDataSource::SocketUpdate(){
        sockaddr addr;
        socklen_t len;
        char buff[1024];
        // getLogger().info("I will update!");
        // handle hello send
        if(recv_socket >= 0){
            std::lock_guard<std::mutex> lock(this->mutex);

            for(auto it = paired_servers.begin(), end = paired_servers.end(); it != end; ++it){
                time_t now = time(NULL);
                if(now - it->last_alive_time > 20){
                    it->last_alive_time = now;
                    sendto(recv_socket, CLIENT_MSG, strlen(CLIENT_MSG), 0, &(it->addr), sizeof(it->addr));
                    getLogger().info("say hello to the server {}", (void*)&*it);
                }
            }
        }


        int poll_fd_count = 0;
        pollfd fds[5];
        {
            int f = pair_socket;
            if(f != -1){
                fds[poll_fd_count].fd = f;
                fds[poll_fd_count].events = POLLIN,
                fds[poll_fd_count].revents = 0;
                poll_fd_count++;
            }
        }
        {
            int f = recv_socket;
            if(f != -1){
                fds[poll_fd_count].fd = f;
                fds[poll_fd_count].events = POLLIN,
                fds[poll_fd_count].revents = 0;
                poll_fd_count++;
            }
        }
        {
            int f = flush_pipe[0];
            if(f != -1){
                fds[poll_fd_count].fd = f;
                fds[poll_fd_count].events = POLLIN,
                fds[poll_fd_count].revents = 0;
                poll_fd_count++;
            }
        }
        
        int r = poll(fds, poll_fd_count, 10*1000);
        
        if(r < 0){
            getLogger().warn("poll failed({})", errno);
            return;
        }
        if(r == 0){
            return;
        }

        // handle broadcasts
        for(int i=0;i<poll_fd_count;i++){
            if(0 == (fds[i].revents & POLLIN))
                continue;
            
            if(fds[i].fd == flush_pipe[0]){
                read(flush_pipe[0], buff, 1);
            }

            int pkglen;
            if(fds[i].fd == pair_socket
             && (len = sizeof(addr), pkglen = recvfrom(pair_socket, buff,sizeof(buff), 0, &addr, &len), pkglen >= 0)){
                // getLogger().info("received a broadcast.");
                if(0 == strncmp(SERVER_MSG, buff, std::min(strlen(SERVER_MSG), (size_t)pkglen))){
                    std::lock_guard<std::mutex> lock(this->mutex);

                    bool already_exist = false;
                    for(auto it=paired_servers.begin(),end=paired_servers.end();it!=end;++it){
                        if(0 == memcmp(&(it->addr), &addr, std::min((size_t)len, sizeof(addr)))){
                            already_exist = true;
                            break;
                        }
                    }
                    if(!already_exist){
                        paired_servers.push_back({addr, 0, false});
                        getLogger().info("a server has been installed");
                    }
                }
            }else{
                getLogger().info("pair sock recv error");
            }

            // recv device packages
            if(fds[i].fd == recv_socket
                && (len = sizeof(addr), pkglen = recvfrom(recv_socket, buff,sizeof(buff), 0, &addr, &len), pkglen >= 0)){
                std::lock_guard<std::mutex> lock(this->mutex);
                bool is_server_ignored = true;

                for(auto it=paired_servers.begin(), end = paired_servers.end();it!=end;++it){
                    if(0 == memcmp(&(it->addr), &addr, std::min((size_t)len, sizeof(addr)))){
                        if(!it->ignored){
                            is_server_ignored = false;
                            break;
                        }
                    }
                }

                if(is_server_ignored){
                    getLogger().info("a package has been ignored.");
                    return;
                }

                // getLogger().info("received a device packet.");
                int name_end = 0;
                for(int i=0;i<pkglen;i++){
                    if(buff[i] == 0){
                        name_end = i;
                        break;
                    }
                }
                int mac_end = name_end;
                for(int i=name_end + 1; i < pkglen;i++){
                    if(buff[i] == 0){
                        mac_end = i;
                        break;
                    }
                }
                if(buff[name_end] || buff[mac_end] || name_end == mac_end || mac_end + 4 >= pkglen){
                    getLogger().warn("a invalid lan heartbeat lan package detected.({} {} {} {} {})", buff[name_end], buff[mac_end], name_end, mac_end, pkglen);
                    continue;
                }
                char *name = buff;
                char *mac = buff + name_end + 1;
                
                uint8_t * hearts = (uint8_t*)&buff[mac_end+1];
                uint32_t heart = 0;
                for(int i=0;i<4;i++){
                    heart = (heart << 8) | hearts[i];
                }

                auto it = avaliable_devices.find(mac);
                if(it == avaliable_devices.end()){
                    avaliable_devices.insert(std::pair(std::string(mac),HeartBeatLanDevice{
                        .name = name,
                        .mac = mac,
                        .last_data = heart,
                        .last_data_time = time(NULL)
                    }));
                }else{
                    auto & d = it->second;
                    if(!d.ignored){
                        d.last_data = heart;
                        d.last_data_time = time(NULL);

                        if(selected_mac.length() == 0 || selected_mac == d.mac){
                            the_heart = heart;
                            has_unread_heart_data = true;
                        }
                    }
                }
            }
        }
    }

    void HeartBeatLanDataSource::StopPair(){
        if(pair_socket != -1){
            close(pair_socket);
            pair_socket = -1;
            getLogger().info("pair socket closed.");
        }
    }

    const char * HeartBeatLanDataSource::GetProtocolVersion(){
        return PROTOCOL_VER;
    }
    bool HeartBeatLanDataSource::GetData(int& heartbeat){
        if(has_unread_heart_data){
            has_unread_heart_data = false;
            heartbeat = the_heart;
            return true;
        }
        return false;
    }
}