namespace HeartBeat{
    namespace ApiInternal{
        // call this function every frame at least once if you need to use the api
        void Update();
        bool GetData(int * heartbeat);
    }
}