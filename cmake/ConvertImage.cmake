include_guard(GLOBAL)

set(CPP_FILE ${CMAKE_CURRENT_BINARY_DIR}/convertimage.cpp)

if(NOT EXISTS ${CPP_FILE})
    file(WRITE ${CPP_FILE} "
        #include <QGuiApplication>
        #include <QIcon>
        #include <QImageReader>
        int main(int argc, char* argv[])
        {
            qputenv(\"QT_QPA_PLATFORM\", \"offscreen\");
            qputenv(\"QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM\", \"true\");
            QGuiApplication a(argc, argv);
            QString i(argv[1]), o(argv[2]), s(argc < 4 ? \"-1\" : argv[3]);
            return !QIcon(i).pixmap(QImageReader(i).size(), s.toDouble()).save(o);
        }
    ")
endif()

function(convert_image INPUT OUTPUT #[[SCALE_FACTOR]])
    if(EXISTS ${OUTPUT})
        return()
    endif()

    message(STATUS "Converting image from ${INPUT} to ${OUTPUT}")

    try_run(RUN_RESULT COMPILE_RESULT
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CPP_FILE}
        LINK_LIBRARIES Qt::Svg
        COMPILE_OUTPUT_VARIABLE COMPILE_OUTPUT
        RUN_OUTPUT_VARIABLE RUN_OUTPUT
        WORKING_DIRECTORY ${QT_BINDIR}
        ARGS ${INPUT} ${OUTPUT} ${ARGN}
    )

    if(NOT COMPILE_RESULT)
        message(FATAL_ERROR ${COMPILE_OUTPUT})
    endif()

    if(NOT RUN_RESULT STREQUAL 0)
        message(FATAL_ERROR ${RUN_OUTPUT})
    endif()
endfunction()
