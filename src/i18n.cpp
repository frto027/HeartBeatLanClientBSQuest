#include "i18n.hpp"
#include "BGLib/Polyglot/Language.hpp"
#include "BGLib/Polyglot/Localization.hpp"
#include "ModConfig.hpp"
#include "BGLib/Polyglot/Localization.hpp"
#include "BGLib/Polyglot/LocalizationModel.hpp"
#include "main.hpp"

Lang *LANG;

namespace I18N {
    void Setup(){
        std::string lang = getModConfig().ModLang.GetValue();
        // if(lang == "auto"){
        //     auto instance = BGLib::Polyglot::Localization::get_Instance();
        //     if(instance){ //FIXME: the instance is always nullptr
        //         switch (BGLib::Polyglot::Localization::get_Instance()->SelectedLanguage) {
        //             case BGLib::Polyglot::Language::Simplified_Chinese:
        //                 lang = "chinese";
        //                 break;
        //             default:
        //                 lang = "english";
        //                 break;
        //         }
        //     }
        // }
        LANG = new Lang();
        #define V(key, value) LANG->key = value;
        #include "langs/english.inc"

        if(lang == "chinese"){
            #include "langs/chinese.inc"
        }
    }
}