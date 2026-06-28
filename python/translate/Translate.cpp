#include "modules/Translate.h"
#include "translate.h"

PyTranslate py_translate{};

void PyTranslate::include(py::module_ &m) {
  DO_INCLUDE()
  py_translate.include(m);

  py::enum_<Languages> l_enum(m, "Languages");
  l_enum.value("None_", Languages::None)
      .value("English", Languages::English)
      .value("French", Languages::French)
      .value("Italian", Languages::Italian)
      .value("German", Languages::German)
      .value("Spanish", Languages::Spanish)
      .value("Russian", Languages::Russian)
      .value("Polish", Languages::Polish)
      .value("Czech", Languages::Czech)
      .value("Turkish", Languages::Turkish)
      .value("Chinese", Languages::Chinese)
      .value("Japan", Languages::Japan)
      .value("Portuguese", Languages::Portuguese)
      .value("Ukrainian", Languages::Ukrainian)
      .value("Serbian", Languages::Serbian)
      .value("Hungarian", Languages::Hungarian)
      .value("Korean", Languages::Korean)
      .value("Belarusian", Languages::Belarusian)
      .value("Romanian", Languages::Romanian)
      .value("TChinese", Languages::TChinese)
      .value("HChinese", Languages::HChinese)
      .value("Vietnamese", Languages::Vietnamese)
      .value("Comments", Languages::Comments)
      .value("max_chars", Languages::max_chars)
      .value("LENGTH", Languages::LENGTH);

  py::enum_<translate::ReturnPolicy> t_enum(m, "ReturnPolicy");
  t_enum.value("ReturnKey", translate::ReturnPolicy::ReturnKey)
      .value("ReturnNull", translate::ReturnPolicy::ReturnNull);

  t_enum.def("__repr__", [](translate::ReturnPolicy &policy) {
    switch (policy) {
      case translate::ReturnPolicy::ReturnKey:
        return std::string("PyReplayParser.ReturnPolicy.ReturnKey");
      case translate::ReturnPolicy::ReturnNull:
        return std::string("PyReplayParser.ReturnPolicy.ReturnNull");
      default:
        return std::string("PyReplayParser.ReturnPolicy.ReturnKey");
    }
  });
  t_enum.def("__str__", [](translate::ReturnPolicy &policy) {
    switch (policy) {
      case translate::ReturnPolicy::ReturnKey:
        return std::string("PyReplayParser.ReturnPolicy.ReturnKey");
      case translate::ReturnPolicy::ReturnNull:
        return std::string("PyReplayParser.ReturnPolicy.ReturnNull");
      default:
        return std::string("PyReplayParser.ReturnPolicy.ReturnKey");
    }
  });

  /// std::string_view value, Languages lang, ReturnPolicy return_policy
  ///
  m.def("localize", &translate::localize, py::arg("value"), py::arg_v("lang", Languages::None, "Languages.None_"),
        py::arg_v("return_policy", translate::ReturnPolicy::ReturnKey, "ReturnPolicy.ReturnKey"),
        "Gets a key from the loaded table based on your selected language.\n"
        "If no key is found, it returns based on the specified return policy.\n"
        "@param value: The key to look up.\n"
        "@param lang: The optional language to use. If no language is passed, then it uses the language set via "
        "set_default_language.\n"
        "@param return_policy: The optional return policy to use. Default is ReturnKey.\n"
        "@return: Returns your translated key or nullptr if not found.");

  m.def("localize_rKey", &translate::localize_rKey, py::arg("value"),
        py::arg_v("lang", Languages::None, "Languages.None_"),
        "See translate::localize\n. explicitly calls localize with ReturnPolicy::ReturnKey.\n");

  m.def("localize_rNull", &translate::localize_rNull, py::arg("value"),
        py::arg_v("lang", Languages::None, "Languages.None_"),
        "See translate::localize\n. explicitly calls localize with ReturnPolicy::ReturnNull.\n");

  m.def("set_default_language", &translate::set_default_language, py::arg("lang"),
        "Sets the default language for translation lookups.\n"
        "@param lang: The language to set as default.");
}
