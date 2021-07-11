find_package(Qt5 QUIET REQUIRED NO_MODULE COMPONENTS Core)
get_target_property(rcc Qt5::rcc LOCATION)
STRING(REPLACE "rcc" "lupdate" lupdate_command ${rcc})
STRING(REPLACE "rcc" "lrelease" lrelease_command ${rcc})
STRING(REPLACE "rcc" "lconvert" lconvert_command ${rcc})

function(LUPDATE Arg_TARGET Arg_Source Arg_Filename)
    add_custom_command(
            TARGET ${Arg_TARGET}
            PRE_BUILD
            COMMAND ${lupdate_command} ${Arg_Source} -ts ${Arg_Filename}
            )
endfunction(LUPDATE)

function(UPDATE_TRANSLATION Arg_TARGET Arg_PrimaryTS Arg_TsDir)
    file(GLOB_RECURSE TS_LIST ${Arg_TsDir}/*.ts)
    LIST(REMOVE_ITEM TS_LIST ${Arg_PrimaryTS})
    foreach (file ${TS_LIST})
        add_custom_command(
                TARGET ${Arg_TARGET}
                PRE_BUILD
                COMMAND ${lconvert_command} -i ${Arg_PrimaryTS} ${file} -o ${file}
                )
    endforeach ()
endfunction(UPDATE_TRANSLATION)

function(COMPILE_TS Arg_TARGET Arg_TsDir Arg_DesDir)
    file(GLOB_RECURSE TS_LIST ${Arg_TsDir}/*.ts)
    foreach(file ${TS_LIST})
        STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" outfile ${file})
        add_custom_command(
                TARGET ${Arg_TARGET}
                PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory ${Arg_DesDir}
                COMMAND ${lrelease_command} ${file} -qm ${Arg_DesDir}/${outfile}.qm
        )
    endforeach()
endfunction(COMPILE_TS)