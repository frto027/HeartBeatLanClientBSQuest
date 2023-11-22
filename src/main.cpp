#include "main.hpp"

static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup

// Loads the config from disk using our modInfo, then returns it for use
// other config tools such as config-utils don't use this config, so it can be removed if those are in use
Configuration& getConfig() {
    static Configuration config(modInfo);
    return config;
}

// Returns a logger, useful for printing debug messages
Logger& getLogger() {
    static Logger* logger = new Logger(modInfo);
    return *logger;
}

// Called at the early stages of game loading
extern "C" void setup(ModInfo& info) {
    info.id = MOD_ID;
    info.version = VERSION;
    modInfo = info;
	
    getConfig().Load();
    getLogger().info("Completed setup!");
}

// parameters are (namespace, class name, parent class, contents)
DECLARE_CLASS_CODEGEN(HeartBeatObject, HeartBeatObj, UnityEngine::MonoBehaviour,
    // DECLARE_INSTANCE_METHOD creates methods
    DECLARE_INSTANCE_METHOD(void, Start);
    DECLARE_INSTANCE_METHOD(void, Update);

    // DECLARE_INSTANCE_FIELD creates fields
    //DECLARE_INSTANCE_FIELD(int, counts);
    TMPro::TextMeshPro* text;
)

DEFINE_TYPE(HeartBeatObject, HeartBeatObj);

namespace HeartBeatObject{
    static int still_live = 0;

    void HeartBeatObj::Start(){
        text = this->get_gameObject()->AddComponent<TMPro::TextMeshPro*>();
        if(text == nullptr){
            getLogger().info("the text create failed.!");
        }else{
            auto rectTransform = text->get_rectTransform();
            rectTransform->set_position({0,2.5,3.5});
            text->set_alignment(TMPro::TextAlignmentOptions::Center);
            text->set_fontSize(3);
            text->set_text("heart heart heart");
        }
    }
    void HeartBeatObj::Update(){
        char buff[1024];
        sprintf(buff, "heart % 3d", still_live++ % 200);
        text->set_text(buff);
    }
};

MAKE_HOOK_MATCH(HeartBeatUIInit, &HMUI::HierarchyManager::Start, void,HMUI::HierarchyManager * self){
    HeartBeatUIInit(self);
    
    getLogger().info("The hook point start!");

    static bool init = false;
    if(init)
        return;
    init = true;
   auto obj = UnityEngine::GameObject::New_ctor("the_heartbeat");
   UnityEngine::GameObject::DontDestroyOnLoad(obj);
   obj->AddComponent<HeartBeatObject::HeartBeatObj*>();

   getLogger().info("Heart object created.");

}
// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();
    QuestUI::Init();
    
    getLogger().info("Installing hooks...");
    INSTALL_HOOK(getLogger(), HeartBeatUIInit);
    getLogger().info("Installed all hooks!");
}