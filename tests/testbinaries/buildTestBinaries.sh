#!/bin/bash

## this script is used to create some mach-o binaries

cc=clang
lipo=lipo
name=testprog
archs=$(cc -print-targets)
targetDir=./ #../testdata/


# replace tool if not found
$($cc 1>/dev/null )
if [[ $? -eq 127 ]]; then
    cc=gcc
fi
$(lipo 1>/dev/null )
if [[ $? -eq 127 ]]; then
    lipo=llvm-lipo
fi

files=()

function buildFiles() {
    arr=("$@")
    for cmd in "${arr[@]}"; do
        echo "$cmd" 
        $($cmd 2>/dev/null )
        if [[ $? -ne 0 ]]; then
            echo "Failed $cmd"
            return false
        fi
    done
}

function comp() {
    target=$1
    a=$2
    echo "$cc "

    cmds=("$cc foolib/foo.c $target -shared  -o foolib/libfoo.$a.dylib"
            "$cc barlib/bar.c $target -shared  -o barlib/libbar.$a.dylib"
            "$cc $name.c $target -Ifoolib -Ibarlib -Lfoolib -Lbarlib -lfoo.$a -lbar.$a -o $targetDir$name.$a"
    )
    buildFiles "${cmds[@]}"

    res="$?"
        echo "res $res"
    if [[ $res == 0 ]]; then
        echo  "hello.$a is arch $a"
        files+=( "$targetDir$name.$a" )
    fi
}

# compile with all available targets
for VAR in "$archs"; do
    for a in $(echo "$VAR" | awk '{print $1}' | awk 'NR>1' ); do
        effTriple=$(cc -print-effective-triple -arch ${a//x86-/x86_})
        if [ "${effTriple:0:7}" != "unknown" ]; then
            target="-target $effTriple" 
            comp "$target" "$a"
        else
            echo "Skipping $a (can't compile it on your system)"
        fi
        echo "-------------"
    done
done

# create at fat object
lipoCmd="$lipo  ${files[@]} -create -output $targetDir$name.fat"
echo "$lipoCmd"
$($lipoCmd)

# create iphone 
sdks=("iphoneos" "watchos" "tvos" "visionos")
for sdk in "${sdks[@]}"  ; do
    $(xcrun -sdk "$sdk" clang -v &>/dev/null)
    if [[ $? -eq 0 ]]; then
        cc="xcrun -sdk $sdk clang"
        comp "" "$sdk"
    fi
done
