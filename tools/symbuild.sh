#!/bin/bash
# Minimal EKA1 ARMI .app builder (replicates makmake/cl_gcc.pm steps)
# usage: symbuild.sh NAME UID3 OUTDIR "src1.cpp src2.cpp ..." "incdirs" "libs" [rss]
set -e
NAME=$1; UID3=$2; OUT=$3; SRCS=$4; INCS=$5; LIBS=$6; RSS=$7
EPOC=${EPOC:-/home/claude/sym/s80_20/epoc32}
T=${TOOLS:-/home/claude/sym/gcc539/bin}
export PATH=$T:$PATH
P=arm-epoc-pe-
mkdir -p $OUT/obj
INC="-I $EPOC/include"
for i in $INCS; do INC="-I $i $INC"; done
DEFS="-D__SYMBIAN32__ -D__GCC32__ -D__EPOC32__ -D__MARM__ -D__MARM_ARMI__ -D__DLL__ -DNDEBUG -D_UNICODE"
CFLAGS="-march=armv4t -mthumb-interwork -pipe -c -nostdinc -Wall -Wno-ctor-dtor-privacy -Wno-unknown-pragmas -s -fomit-frame-pointer ${OPT:--O2} $DEFS -include $EPOC/include/variant/symbian_os_v7.0s.hrh"
if [ -n "$RSS" ]; then
  R=$(basename $RSS .rss | tr A-Z a-z)
  ${P}cpp -nostdinc -undef -C -D_UNICODE -I $(dirname $RSS) $INC -I $EPOC/include $RSS > $OUT/obj/$R.rpp
  rcomp -u -o$OUT/$R.rsc -h$OUT/obj/$R.rsg -s$OUT/obj/$R.rpp -i$RSS
  INC="-I $OUT/obj $INC"
fi
OBJS=""
for s in $SRCS; do
  o=$OUT/obj/$(basename $s .cpp).o
  ${P}gcc $CFLAGS $INC -o $o $s
  OBJS="$OBJS $o"
done
L=$EPOC/release/armi/urel
rm -f $OUT/obj/$NAME.in; ${P}ar -cr $OUT/obj/$NAME.in $OBJS
LINKAS="$NAME[$(echo ${UID3#0x} | tr A-F a-f)].app"
printf 'EXPORTS\n\tNewApplication__Fv @ 1 NONAME ; NewApplication(void)\n' > $OUT/obj/$NAME.def
LIBPATHS="$L/edllstub.lib $L/egcc.lib"
for l in $LIBS; do LIBPATHS="$LIBPATHS $L/$l"; done
cd $OUT/obj
${P}dlltool -m arm_interwork --def $NAME.def --output-exp $NAME.exp --dllname "$LINKAS"
${P}ld -s -e _E32Dll -u _E32Dll $NAME.exp --dll --base-file $NAME.bas -o $NAME.tmp $L/edll.lib --whole-archive $NAME.in --no-whole-archive $LIBPATHS
rm -f $NAME.exp $NAME.tmp
${P}dlltool -m arm_interwork --def $NAME.def --dllname "$LINKAS" --base-file $NAME.bas --output-exp $NAME.exp
${P}ld -s -e _E32Dll -u _E32Dll --dll $NAME.exp -Map $NAME.map -o $NAME.tmp $L/edll.lib --whole-archive $NAME.in --no-whole-archive $LIBPATHS
petran $NAME.tmp ../$NAME.app -nocall -uid1 0x10000079 -uid2 0x100039ce -uid3 $UID3 | grep -v "^$" | tail -3
ls -la ../$NAME.app
