include_guard(GLOBAL)

include(ConvertImage)
include(FetchContent)

if(QT_FEATURE_openssl)
    if(ANDROID)
        FetchContent_Declare(
          android_openssl
          GIT_REPOSITORY https://github.com/KDAB/android_openssl
          GIT_TAG aef7ae266e2742328fb66a89acb85c0d2b55c599
          GIT_PROGRESS TRUE
        )
        FetchContent_MakeAvailable(android_openssl)
    else()
        find_package(OpenSSL 3 EXACT COMPONENTS SSL)
    endif()
endif()

function(get_sub_targets OUTPUT DIR #[[TYPES...]])
    cmake_parse_arguments(PARSE_ARGV 2 ARG IGNORED1 IGNORED2 TYPES)

    get_directory_property(SUBTARGETS DIRECTORY ${DIR} BUILDSYSTEM_TARGETS)
    foreach(SUBTARGET IN LISTS SUBTARGETS)
        if(SUBTARGET AND TARGET ${SUBTARGET})
            get_target_property(TARGET_TYPE ${SUBTARGET} TYPE)
            if(NOT ARG_TYPES OR TARGET_TYPE IN_LIST ARG_TYPES)
                list(APPEND TARGET_LIST ${SUBTARGET})
            endif()
        endif()
    endforeach()

    get_directory_property(SUBDIRS DIRECTORY ${DIR} SUBDIRECTORIES)
    foreach(SUBDIR IN LISTS SUBDIRS)
        get_sub_targets(SUB_TARGET_LIST ${SUBDIR} TYPES ${ARG_TYPES})
    endforeach()

    if(SUB_TARGET_LIST)
        list(APPEND TARGET_LIST ${SUB_TARGET_LIST})
    endif()

    if(TARGET_LIST)
        list(REMOVE_DUPLICATES TARGET_LIST)
        set(${OUTPUT} "${TARGET_LIST}" PARENT_SCOPE)
    endif()
endfunction()

function(deploy_android TARGET DEPLOY_SOURCE_DIR)
    file(COPY ${DEPLOY_SOURCE_DIR} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    configure_file(${DEPLOY_SOURCE_DIR}/AndroidManifest.xml
        ${CMAKE_CURRENT_BINARY_DIR}/android/AndroidManifest.xml @ONLY)
    set_target_properties(${TARGET} PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/android)

    if(DEFINED ANDROID_EXTRA_LIBS)
        set_property(TARGET ${TARGET} APPEND PROPERTY
            QT_ANDROID_EXTRA_LIBS "${ANDROID_EXTRA_LIBS}")
    endif()

    get_sub_targets(LIBS ${CMAKE_CURRENT_SOURCE_DIR} TYPES MODULE_LIBRARY)
    foreach(LIB IN LISTS LIBS)
        set_property(TARGET ${TARGET} APPEND PROPERTY
            QT_ANDROID_EXTRA_LIBS $<TARGET_FILE:${LIB}>)
    endforeach()

    get_target_property(EXTRA_LIB_PATHS ${TARGET} QT_ANDROID_EXTRA_LIBS)
    if(EXTRA_LIB_PATHS)
        foreach(PATH IN LISTS EXTRA_LIB_PATHS)
            string(GENEX_STRIP ${PATH} PATH_UNGENEXED)
            if(PATH STREQUAL PATH_UNGENEXED)
                file(TO_CMAKE_PATH ${PATH} PATH)
            endif()
            set_property(TARGET ${TARGET} APPEND PROPERTY
                _qt_android_native_extra_libs ${PATH})
        endforeach()
        set_target_properties(${TARGET} PROPERTIES QT_ANDROID_EXTRA_LIBS "")
    endif()

    add_custom_command(TARGET deploy VERBATIM
        COMMAND ${CMAKE_COMMAND} -E rename
        ${CMAKE_CURRENT_BINARY_DIR}/android-build/build/outputs/apk/debug/android-build-debug.apk
        ${APP_DEPLOY_PREFIX}/${TARGET}-${CMAKE_PROJECT_VERSION}.apk
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endfunction()

function(deploy_ios TARGET DEPLOY_SOURCE_DIR)
    set_target_properties(${TARGET} PROPERTIES
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER
        ${CMAKE_PROJECT_HOMEPAGE_URL}.${CMAKE_PROJECT_NAME}
    )
    configure_file(${DEPLOY_SOURCE_DIR}/Info.plist
        ${CMAKE_CURRENT_BINARY_DIR}/Info.plist @ONLY)
    set_target_properties(${TARGET} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)
    install(DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin/${AppName}
            DESTINATION ${APP_DEPLOY_PREFIX} USE_SOURCE_PERMISSIONS)
endfunction()

function(deploy_darwin TARGET DEPLOY_SOURCE_DIR)
    set(DEPLOY_PREFIX_PATH ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.app)

    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS ${QT_BINDIR})

    get_sub_targets(BINS ${CMAKE_CURRENT_SOURCE_DIR} TYPES EXECUTABLE)
    get_sub_targets(LIBS ${CMAKE_CURRENT_SOURCE_DIR} TYPES SHARED_LIBRARY)
    get_sub_targets(PLUGINS ${CMAKE_CURRENT_SOURCE_DIR} TYPES MODULE_LIBRARY)

    file(MAKE_DIRECTORY
        ${DEPLOY_PREFIX_PATH}/Contents/MacOS
        ${DEPLOY_PREFIX_PATH}/Contents/Frameworks
        ${DEPLOY_PREFIX_PATH}/Contents/PlugIns
        ${DEPLOY_PREFIX_PATH}/Contents/Resources/Translations
    )
    foreach(X ar;bg;ca;cs;da;de;en;es;fa;fi;fr;gd;he;hr;hu;it
              ja;ko;lv;nl;nn;pl;pt_BR;ru;sk;tr;uk;zh_CN;zh_TW)
        file(MAKE_DIRECTORY ${DEPLOY_PREFIX_PATH}/Contents/Resources/${X}.lproj)
    endforeach()

    configure_file(${DEPLOY_SOURCE_DIR}/background.svg
        ${CMAKE_CURRENT_BINARY_DIR}/background.svg @ONLY)
    convert_image(${DEPLOY_SOURCE_DIR}/../setup.svg
                  ${CMAKE_CURRENT_BINARY_DIR}/setup.icns)
    convert_image(${CMAKE_CURRENT_BINARY_DIR}/background.svg
                  ${CMAKE_CURRENT_BINARY_DIR}/background.png)
    convert_image(${CMAKE_CURRENT_BINARY_DIR}/background.svg
                  ${CMAKE_CURRENT_BINARY_DIR}/background@2x.png 2)
    convert_image(${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/icon.svg
                  ${DEPLOY_PREFIX_PATH}/Contents/Resources/icon.icns)

    configure_file(${DEPLOY_SOURCE_DIR}/Info.plist
        ${DEPLOY_PREFIX_PATH}/Contents/Info.plist @ONLY)

    foreach(BIN IN LISTS BINS)
        get_target_property(QM_FILES ${BIN} QM_FILES)
        if(QM_FILES)
            add_custom_command(TARGET deploy_base VERBATIM
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${QM_FILES} ${DEPLOY_PREFIX_PATH}/Contents/Resources/Translations
            )
        endif()
        add_custom_command(TARGET deploy_base VERBATIM
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${BIN}> ${DEPLOY_PREFIX_PATH}/Contents/MacOS
        )
    endforeach()

    foreach(LIB IN LISTS LIBS)
        get_target_property(QM_FILES ${LIB} QM_FILES)
        if(QM_FILES)
            add_custom_command(TARGET deploy_base VERBATIM
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${QM_FILES} ${DEPLOY_PREFIX_PATH}/Contents/Resources/Translations
            )
        endif()
        get_target_property(IS_FRAMEWORK ${LIB} FRAMEWORK)
        if(IS_FRAMEWORK)
            add_custom_command(TARGET deploy_base VERBATIM
                COMMAND rm -rf
                ${DEPLOY_PREFIX_PATH}/Contents/Frameworks/$<TARGET_BUNDLE_DIR_NAME:${LIB}>
            )
            add_custom_command(TARGET deploy_base VERBATIM
                COMMAND cp -a
                $<TARGET_BUNDLE_DIR:${LIB}> ${DEPLOY_PREFIX_PATH}/Contents/Frameworks
            )
        else()
            add_custom_command(TARGET deploy_base VERBATIM
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:${LIB}> ${DEPLOY_PREFIX_PATH}/Contents/Frameworks
            )
        endif()
    endforeach()

    foreach(PLUGIN IN LISTS PLUGINS)
        get_target_property(QM_FILES ${PLUGIN} QM_FILES)
        if(QM_FILES)
            add_custom_command(TARGET deploy_base VERBATIM
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${QM_FILES} ${DEPLOY_PREFIX_PATH}/Contents/Resources/Translations
            )
        endif()
        get_target_property(PLUGIN_TYPE ${PLUGIN} PLUGIN_TYPE)
        add_custom_command(TARGET deploy_base VERBATIM
            COMMAND ${CMAKE_COMMAND} -E make_directory
            ${DEPLOY_PREFIX_PATH}/Contents/PlugIns/${PLUGIN_TYPE}
        )
        add_custom_command(TARGET deploy_base VERBATIM
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${PLUGIN}> ${DEPLOY_PREFIX_PATH}/Contents/PlugIns/${PLUGIN_TYPE}
        )
    endforeach()

    if(OPENSSL_FOUND)
        foreach(LIB IN LISTS OPENSSL_SSL_LIBRARY OPENSSL_CRYPTO_LIBRARY)
            file(REAL_PATH ${LIB} LIB)
            add_custom_command(TARGET deploy_base VERBATIM
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${LIB} ${DEPLOY_PREFIX_PATH}/Contents/Frameworks
            )
        endforeach()
    endif()

    add_custom_command(TARGET deploy VERBATIM
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DEPLOY_SOURCE_DIR}/qt.conf ${DEPLOY_PREFIX_PATH}/Contents/Resources
    )

    add_custom_command(TARGET deploy VERBATIM
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${DEPLOY_PREFIX_PATH} ${APP_DEPLOY_PREFIX}/${TARGET}.app
    )

    add_custom_command(TARGET deploy VERBATIM
        COMMAND ${MACDEPLOYQT_EXECUTABLE}
        ${TARGET}.app -hardened-runtime -appstore-compliant
        WORKING_DIRECTORY ${APP_DEPLOY_PREFIX}
    )

    add_custom_command(TARGET deploy VERBATIM
        COMMAND tiffutil
        -cathidpicheck background.png background@2x.png
        -out background.tiff
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    add_custom_command(TARGET deploy VERBATIM
        COMMAND osascript
        ${DEPLOY_SOURCE_DIR}/SetupDMG.applescript
        ${APP_DEPLOY_PREFIX}
        ${TARGET}.app
        ${TARGET}
        ${CMAKE_CURRENT_BINARY_DIR}/setup.icns
        ${CMAKE_CURRENT_BINARY_DIR}/background.tiff
        12 96 400 200 640 360
        WORKING_DIRECTORY ${APP_DEPLOY_PREFIX}
    )

    add_custom_command(TARGET deploy VERBATIM
        COMMAND ${CMAKE_COMMAND} -E rename
        ${TARGET}.dmg
        ${TARGET}-${CMAKE_PROJECT_VERSION}-${APP_SYSTEM_PROCESSOR}.dmg
        WORKING_DIRECTORY ${APP_DEPLOY_PREFIX}
    )
endfunction()

function(deploy_linux TARGET DEPLOY_SOURCE_DIR)
    set(DEPLOY_PREFIX_PATH ${APP_DEPLOY_PREFIX}/${TARGET}.AppDir)

    find_program(LINUXDEPLOYQT_EXECUTABLE linuxdeployqt)

    if(NOT LINUXDEPLOYQT_EXECUTABLE)
        message(STATUS "Could NOT find linuxdeployqt, downloading...")
        set(LINUXDEPLOYQT_EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/linuxdeployqt)
        set(LINUXDEPLOYQT_URL "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage")
        file(DOWNLOAD ${LINUXDEPLOYQT_URL} ${LINUXDEPLOYQT_EXECUTABLE} SHOW_PROGRESS)
        file(CHMOD ${LINUXDEPLOYQT_EXECUTABLE} PERMISSIONS OWNER_READ OWNER_EXECUTE)
    endif()

    get_sub_targets(BINS ${CMAKE_CURRENT_SOURCE_DIR} TYPES EXECUTABLE)
    get_sub_targets(LIBS ${CMAKE_CURRENT_SOURCE_DIR} TYPES SHARED_LIBRARY)
    get_sub_targets(PLUGINS ${CMAKE_CURRENT_SOURCE_DIR} TYPES MODULE_LIBRARY)

    add_custom_command(TARGET deploy VERBATIM
        COMMAND ${CMAKE_COMMAND} -E make_directory
        ${DEPLOY_PREFIX_PATH}/usr/bin
        ${DEPLOY_PREFIX_PATH}/usr/lib
        ${DEPLOY_PREFIX_PATH}/usr/translations
    )

    configure_file(${DEPLOY_SOURCE_DIR}/icon.desktop
        ${DEPLOY_PREFIX_PATH}/usr/share/applications/${TARGET}.desktop @ONLY)

    file(MAKE_DIRECTORY ${DEPLOY_PREFIX_PATH}/usr/share/icons/hicolor/128x128/apps)
    file(MAKE_DIRECTORY ${DEPLOY_PREFIX_PATH}/usr/share/icons/hicolor/scalable/apps)
    file(COPY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/icon.svg
        ${DEPLOY_PREFIX_PATH}/usr/share/icons/hicolor/scalable/apps/${TARGET}.svg)
    convert_image(${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/icon.svg
                  ${DEPLOY_PREFIX_PATH}/usr/share/icons/hicolor/128x128/apps/${TARGET}.png)
    file(CREATE_LINK usr/share/applications/${TARGET}.desktop
        ${DEPLOY_PREFIX_PATH}/${TARGET}.desktop SYMBOLIC)
    file(CREATE_LINK usr/share/icons/hicolor/128x128/apps/${TARGET}.png
        ${DEPLOY_PREFIX_PATH}/${TARGET}.png SYMBOLIC)
    file(CREATE_LINK usr/share/icons/hicolor/128x128/apps/${TARGET}.png
        ${DEPLOY_PREFIX_PATH}/.DirIcon SYMBOLIC)

    # Copy the binary to the bin directory
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/wt_ext_cli/wt_ext_cli
        DESTINATION ${DEPLOY_PREFIX_PATH}/usr/bin)

    foreach(BIN IN LISTS BINS)
        get_target_property(QM_FILES ${BIN} QM_FILES)
        if(QM_FILES)
            add_custom_command(TARGET deploy VERBATIM
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${QM_FILES} ${DEPLOY_PREFIX_PATH}/usr/translations
            )
        endif()
        add_custom_command(TARGET deploy VERBATIM
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${BIN}> ${DEPLOY_PREFIX_PATH}/usr/bin
        )
        add_custom_command(TARGET deploy_base VERBATIM
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${BIN}> ${CMAKE_CURRENT_BINARY_DIR}
        )
    endforeach()

    foreach(LIB IN LISTS LIBS)
        get_target_property(QM_FILES ${LIB} QM_FILES)
        if(QM_FILES)
            add_custom_command(TARGET deploy VERBATIM
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${QM_FILES} ${DEPLOY_PREFIX_PATH}/usr/translations
            )
        endif()
        add_custom_command(TARGET deploy VERBATIM
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${LIB}> ${DEPLOY_PREFIX_PATH}/usr/lib
        )
    endforeach()

    foreach(PLUGIN IN LISTS PLUGINS)
        get_target_property(QM_FILES ${PLUGIN} QM_FILES)
        if(QM_FILES)
            add_custom_command(TARGET deploy VERBATIM
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${QM_FILES} ${DEPLOY_PREFIX_PATH}/usr/translations
            )
        endif()
        get_target_property(PLUGIN_TYPE ${PLUGIN} PLUGIN_TYPE)
        add_custom_command(TARGET deploy VERBATIM
            COMMAND ${CMAKE_COMMAND} -E make_directory
            ${DEPLOY_PREFIX_PATH}/usr/plugins/${PLUGIN_TYPE}
        )
        add_custom_command(TARGET deploy VERBATIM
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${PLUGIN}> ${DEPLOY_PREFIX_PATH}/usr/plugins/${PLUGIN_TYPE}
        )
        add_custom_command(TARGET deploy_base VERBATIM
            COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_TYPE}
        )
        add_custom_command(TARGET deploy_base VERBATIM
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${PLUGIN}> ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_TYPE}
        )
    endforeach()

    if(OPENSSL_FOUND)
        foreach(LIB IN LISTS OPENSSL_SSL_LIBRARY OPENSSL_CRYPTO_LIBRARY)
            file(REAL_PATH ${LIB} LIB)
            add_custom_command(TARGET deploy VERBATIM
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${LIB} ${DEPLOY_PREFIX_PATH}/usr/lib
            )
            add_custom_command(TARGET deploy_base VERBATIM
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${LIB} ${CMAKE_CURRENT_BINARY_DIR}
            )
        endforeach()
    endif()

    add_custom_command(TARGET deploy VERBATIM
        COMMAND VERSION=${CMAKE_PROJECT_VERSION} ARCH=${APP_SYSTEM_PROCESSOR} ${LINUXDEPLOYQT_EXECUTABLE}
        ${DEPLOY_PREFIX_PATH}/usr/bin/$<TARGET_FILE_NAME:${TARGET}>
        -appimage
        -qmake=${QMAKE_EXECUTABLE}
        -extra-plugins=platformthemes,tls
        WORKING_DIRECTORY ${APP_DEPLOY_PREFIX}
    )
endfunction()

function(deploy_windows TARGET DEPLOY_SOURCE_DIR)
    set(NSIS_DIR ${CMAKE_CURRENT_BINARY_DIR}/nsis-3.08)
    set(DEPLOY_PREFIX_PATH ${APP_DEPLOY_PREFIX}/${TARGET})

    find_program(MAKENSIS_EXECUTABLE makensis HINTS ${NSIS_DIR})
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${QT_BINDIR})

    if(NOT MAKENSIS_EXECUTABLE)
        message(STATUS "Could NOT find NSIS, downloading...")
        set(NSIS_ZIP ${CMAKE_CURRENT_BINARY_DIR}/nsis.zip)
        set(NSIS_URL "https://sourceforge.net/projects/nsis/files/NSIS%203/3.08/nsis-3.08.zip/download")
        file(DOWNLOAD ${NSIS_URL} ${NSIS_ZIP} SHOW_PROGRESS)
        file(ARCHIVE_EXTRACT INPUT ${NSIS_ZIP})
        find_program(MAKENSIS_EXECUTABLE makensis HINTS ${NSIS_DIR})
    endif()

    get_sub_targets(BINS ${CMAKE_CURRENT_SOURCE_DIR} TYPES EXECUTABLE)
    get_sub_targets(LIBS ${CMAKE_CURRENT_SOURCE_DIR} TYPES SHARED_LIBRARY)
    get_sub_targets(PLUGINS ${CMAKE_CURRENT_SOURCE_DIR} TYPES MODULE_LIBRARY)

    add_custom_command(TARGET deploy VERBATIM
        COMMAND ${CMAKE_COMMAND} -E make_directory
        ${DEPLOY_PREFIX_PATH}/translations
    )

    convert_image(${DEPLOY_SOURCE_DIR}/../setup.svg
                  ${CMAKE_CURRENT_BINARY_DIR}/setup.ico)
    convert_image(${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/icon.svg
                  ${CMAKE_CURRENT_BINARY_DIR}/icon.ico)

    file(COPY ${DEPLOY_SOURCE_DIR}/qt.conf DESTINATION ${DEPLOY_PREFIX_PATH})
    file(COPY ${DEPLOY_SOURCE_DIR}/icon.rc DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    target_sources(${TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/icon.rc)

    set(CPACK_GENERATOR NSIS CACHE STRING " ")
    set(CPACK_PACKAGE_NAME ${TARGET} CACHE STRING " ")
    set(CPACK_PACKAGE_FILE_NAME ${TARGET}-${CMAKE_PROJECT_VERSION}-${APP_SYSTEM_PROCESSOR} CACHE STRING " ")
    set(CPACK_PACKAGE_VENDOR ${CMAKE_PROJECT_HOMEPAGE_URL} CACHE STRING " ")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME} CACHE STRING " ")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${CMAKE_PROJECT_DESCRIPTION} CACHE STRING " ")
    set(CPACK_INSTALLED_DIRECTORIES ${DEPLOY_PREFIX_PATH} . CACHE STRING " ")
    set(CPACK_PACKAGE_DIRECTORY ${APP_DEPLOY_PREFIX} CACHE PATH " ")
    set(CPACK_PACKAGE_DESCRIPTION_FILE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt CACHE PATH " ")
    set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt CACHE PATH " ")
    set(CPACK_PACKAGE_EXECUTABLES ${TARGET} ${TARGET} CACHE STRING " ")
    set(CPACK_NSIS_EXECUTABLE ${MAKENSIS_EXECUTABLE} CACHE PATH " ")
    set(CPACK_NSIS_URL_INFO_ABOUT ${CMAKE_PROJECT_HOMEPAGE_URL} CACHE STRING " ")
    set(CPACK_NSIS_MUI_ICON ${CMAKE_CURRENT_BINARY_DIR}/setup.ico CACHE PATH " ")
    set(CPACK_NSIS_MUI_UNIICON ${CMAKE_CURRENT_BINARY_DIR}/setup.ico CACHE PATH " ")
    set(CPACK_NSIS_INSTALLED_ICON_NAME Uninstall.exe CACHE STRING " ")
    set(CPACK_NSIS_MANIFEST_DPI_AWARE TRUE CACHE BOOL " ")
    set(CPACK_NSIS_BRANDING_TEXT ${CMAKE_PROJECT_HOMEPAGE_URL} CACHE STRING " ")
    set(CPACK_NSIS_EXECUTABLES_DIRECTORY . CACHE STRING " ")
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "CreateShortCut '$DESKTOP\\\\${TARGET}.lnk' '$INSTDIR\\\\${TARGET}.exe'" CACHE STRING " ")
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Delete '$DESKTOP\\\\${TARGET}.lnk'" CACHE STRING " ")

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/qt.conf "[Platforms]
        WindowsArguments = fontengine=freetype")

    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/wt_ext_cli/wt_ext_cli.exe
        DESTINATION ${DEPLOY_PREFIX_PATH})

    foreach(BIN IN LISTS BINS)
        get_target_property(QM_FILES ${BIN} QM_FILES)
        if(QM_FILES)
            add_custom_command(TARGET deploy VERBATIM
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${QM_FILES} ${DEPLOY_PREFIX_PATH}/translations
            )
        endif()
        add_custom_command(TARGET deploy VERBATIM
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${BIN}> ${DEPLOY_PREFIX_PATH}
        )
        add_custom_command(TARGET deploy_base VERBATIM
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${BIN}> ${CMAKE_CURRENT_BINARY_DIR}
        )
    endforeach()

    foreach(LIB IN LISTS LIBS)
        get_target_property(QM_FILES ${LIB} QM_FILES)
        if(QM_FILES)
            add_custom_command(TARGET deploy VERBATIM
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${QM_FILES} ${DEPLOY_PREFIX_PATH}/translations
            )
        endif()
        add_custom_command(TARGET deploy VERBATIM
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${LIB}> ${DEPLOY_PREFIX_PATH}
        )
    endforeach()

    foreach(PLUGIN IN LISTS PLUGINS)
        get_target_property(QM_FILES ${PLUGIN} QM_FILES)
        if(QM_FILES)
            add_custom_command(TARGET deploy VERBATIM
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${QM_FILES} ${DEPLOY_PREFIX_PATH}/translations
            )
        endif()
        get_target_property(PLUGIN_TYPE ${PLUGIN} PLUGIN_TYPE)
        add_custom_command(TARGET deploy VERBATIM
            COMMAND ${CMAKE_COMMAND} -E make_directory
            ${DEPLOY_PREFIX_PATH}/plugins/${PLUGIN_TYPE}
        )
        add_custom_command(TARGET deploy VERBATIM
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${PLUGIN}> ${DEPLOY_PREFIX_PATH}/plugins/${PLUGIN_TYPE}
        )
        add_custom_command(TARGET deploy_base VERBATIM
            COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_TYPE}
        )
        add_custom_command(TARGET deploy_base VERBATIM
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${PLUGIN}> ${CMAKE_CURRENT_BINARY_DIR}/${PLUGIN_TYPE}
        )
    endforeach()

    if(OPENSSL_FOUND)
        list(GET OPENSSL_LIBRARIES 1 LIB)
        file(REAL_PATH "${LIB}/../../bin" LIB)
        if(NOT EXISTS ${LIB})
            file(REAL_PATH "${LIB}/../../bin" LIB)
        endif()
        file(GLOB DLLS "${LIB}/lib*.dll")
        foreach(DLL IN LISTS DLLS)
            add_custom_command(TARGET deploy VERBATIM
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${DLL} ${DEPLOY_PREFIX_PATH}
            )
            add_custom_command(TARGET deploy_base VERBATIM
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${DLL} ${CMAKE_CURRENT_BINARY_DIR}
            )
        endforeach()
    endif()

    if(MSVC)
        add_custom_command(TARGET deploy VERBATIM
            COMMAND ${WINDEPLOYQT_EXECUTABLE}
            --no-compiler-runtime
            --no-opengl-sw
            $<IF:$<CONFIG:Debug>,--debug,--release>
            ${DEPLOY_PREFIX_PATH}/$<TARGET_FILE_NAME:${TARGET}>
            WORKING_DIRECTORY ${DEPLOY_PREFIX_PATH}
        )
        add_custom_command(TARGET deploy VERBATIM
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} ${DEPLOY_PREFIX_PATH}
            COMMAND_EXPAND_LISTS
        )
    else()
        add_custom_command(TARGET deploy VERBATIM
            COMMAND ${WINDEPLOYQT_EXECUTABLE}
            --compiler-runtime
            --no-opengl-sw
            $<IF:$<CONFIG:Debug>,--debug,-v>
            ${DEPLOY_PREFIX_PATH}/$<TARGET_FILE_NAME:${TARGET}>
            WORKING_DIRECTORY ${DEPLOY_PREFIX_PATH}
        )
    endif()

    cmake_language(DEFER CALL include CPack)
    cmake_language(DEFER CALL add_custom_command TARGET deploy VERBATIM
        COMMAND ${CMAKE_CPACK_COMMAND} -C $<CONFIGURATION>
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    cmake_language(DEFER CALL add_custom_command TARGET deploy VERBATIM
        COMMAND ${CMAKE_COMMAND} -E rm -rf _CPack_Packages
        WORKING_DIRECTORY ${APP_DEPLOY_PREFIX}
    )
endfunction()

function(deploy TARGET DEPLOY_BASE_DIR)
    if(ANDROID)
        add_custom_target(deploy_base ALL DEPENDS apk_all)
    else()
        add_custom_target(deploy_base ALL DEPENDS ${TARGET})
    endif()

    if(APP_DEPLOY_AS_PART_OF_ALL)
        add_custom_target(deploy ALL DEPENDS deploy_base)
    else()
        add_custom_target(deploy DEPENDS deploy_base)
    endif()

    set(DEPLOY_SOURCE_DIR ${DEPLOY_BASE_DIR}/${APP_SYSTEM_NAME})
    if(NOT IS_ABSOLUTE ${DEPLOY_BASE_DIR})
        string(JOIN / DEPLOY_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} ${DEPLOY_SOURCE_DIR})
    endif()

    file(MAKE_DIRECTORY ${APP_DEPLOY_PREFIX})

    cmake_language(CALL deploy_${APP_SYSTEM_NAME} ${TARGET} ${DEPLOY_SOURCE_DIR})
endfunction()
