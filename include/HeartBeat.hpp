#include "codegen/include/UnityEngine/MonoBehaviour.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "custom-types/shared/macros.hpp"
#include "codegen/include/TMPro/TextMeshPro.hpp"

// parameters are (namespace, class name, parent class, contents)
DECLARE_CLASS_CODEGEN(HeartBeat, HeartBeatObj, UnityEngine::MonoBehaviour,
    // DECLARE_INSTANCE_METHOD creates methods
    DECLARE_INSTANCE_METHOD(void, Start);
    DECLARE_INSTANCE_METHOD(void, Update);

    // DECLARE_INSTANCE_FIELD creates fields
    //DECLARE_INSTANCE_FIELD(int, counts);
    TMPro::TextMeshPro* text;
)