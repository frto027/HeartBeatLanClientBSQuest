#include "i18n.hpp"
#include "BGLib/Polyglot/zzzz__Language_def.hpp"
#include "BGLib/Polyglot/zzzz__Localization_def.hpp"
#include "ModConfig.hpp"
#include "BGLib/Polyglot/Localization.hpp"
#include "BGLib/Polyglot/LocalizationModel.hpp"

Lang *LANG;

namespace I18N {
    void Setup(){
        std::string lang = getModConfig().ModLang.GetValue();

        if(lang == "auto"){
            switch (BGLib::Polyglot::Localization::get_Instance()->SelectedLanguage) {
                case BGLib::Polyglot::Language::Simplified_Chinese:
                    lang = "chinese";
                    break;
                default:
                    lang = "english";
                    break;
            }
        }

        if(lang == "chinese"){
            LANG = new ChineseLang();
        }else{
            LANG = new Lang();
        }
    }
}