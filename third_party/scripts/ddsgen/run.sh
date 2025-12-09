#! /bin/bash

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
cd $SCRIPT_DIR
IDL_PATH="$SCRIPT_DIR/../../../idl"
#TEMPLATE_ARR=(Header.stg Cdr_source.stg Source.stg Pybind.stg TypeObjectHeader.stg TypeObjectSource.stg)
#FILETYPE_ARR=(.hpp .ipp .cpp .pybind.ipp TypeObject.hpp TypeObject.cpp)
TEMPLATE_ARR=(Header.stg Cdr_source.stg Source.stg Pybind.stg)
FILETYPE_ARR=(.hpp .ipp .cpp .pybind.ipp)
GENDIR="genfiles"
mkdir -p "$IDL_PATH/$GENDIR"

echo "IDL_PATH:$IDL_PATH"

# read -p "Input idl file name:" IDL_NAME
IDL_NAME="$1"
enable_pybind=false
input=${2:-$enable_pybind}
case "$input" in
    1|true|yes|on|TRUE|YES|ON)  enable_pybind=true ;;
    0|false|no|off|FALSE|NO|OFF) enable_pybind=false ;;
    *)
        echo "Error: invalid boolean value '$input'"
        exit 1
        ;;
esac
use_stg_len=2
if [[ "$enable_pybind" == "true" ]]; then
    echo "Enable pybind"
    use_stg_len=3
    flags=(-typeros2)
else
    flags=(-typeros2)
fi


if [ "$IDL_NAME" = "*" ]; then
    echo "Use all files"

    for file in $IDL_PATH/*.idl; do
        if [ -f "$file" ]; then
            filename=$(basename "$file" .idl)
            for i in $(seq 0 $use_stg_len); do
                "$SCRIPT_DIR/scripts/fastddsgen" "${flags[@]}" "$file" -I ./templates -replace  -extrastg ./my_templates/"${TEMPLATE_ARR[i]}" "$filename${FILETYPE_ARR[i]}"
                if [ -f "$IDL_PATH/$filename${FILETYPE_ARR[i]}" ]; then
                    mv "$IDL_PATH/$filename${FILETYPE_ARR[i]}"  "$IDL_PATH/$GENDIR"
                else
                    echo "Generate $IDL_PATH/$filename${FILETYPE_ARR[i]}  error"
                    rm -f $IDL_PATH/*.h $IDL_PATH/*.cxx $IDL_PATH/*CdrAux.ipp $IDL_PATH/*CdrAux.hpp
                    exit
                fi
            done
        fi
    done


else
    if [ -e "$IDL_PATH/$IDL_NAME".idl ]; then
        echo "Find this file"
    else
        echo "This is file not exist"
        exit
    fi
    for i in $(seq 0 $use_stg_len); do
        "$SCRIPT_DIR/scripts/fastddsgen" "${flags[@]}" "$IDL_PATH/$IDL_NAME".idl -I ./templates -replace  -extrastg ./my_templates/"${TEMPLATE_ARR[i]}" "$IDL_NAME${FILETYPE_ARR[i]}"
        if [ -f "$IDL_PATH/$IDL_NAME${FILETYPE_ARR[i]}" ]; then
            mv "$IDL_PATH/$IDL_NAME${FILETYPE_ARR[i]}"  "$IDL_PATH/$GENDIR"
        else
            echo "Generate $IDL_PATH/$IDL_NAME${FILETYPE_ARR[i]} error"
            rm -f $IDL_PATH/*.h $IDL_PATH/*.cxx $IDL_PATH/*CdrAux.ipp $IDL_PATH/*CdrAux.hpp
            exit
        fi

    done
fi

rm -f $IDL_PATH/*.h $IDL_PATH/*.cxx $IDL_PATH/*CdrAux.ipp $IDL_PATH/*CdrAux.hpp
