add_library(PyTranslate Translate.cpp)

target_link_libraries(PyTranslate
        pybind11::module
        Python::Python
        Translate::Translate
)

add_library(PyTranslate::PyTranslate ALIAS PyTranslate)

target_include_directories(PyTranslate PRIVATE
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/python/include"
)