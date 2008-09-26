#!/bin/sh


# Low\High 1209 1336 1447 1633
# 697      [1]  [2]  [3]   [A]
# 770      [4]  [5]  [6]   [B]
# 852      [7]  [8]  [9]   [C]
# 941      [*(e][0]  [#(f] [D]

FILE_PREFIX="dtmf_" 
FILE_SURFIX=".8k16b1chraw" 
FILE_TMP_PREFIX="z.dtmf.tmp."
SPL_TIME="0.15" #second 
SPL_RATE="8000"
#-Word -Char / -Unsigned -Signed /-Network -Intel byte order
SPL_TYPE="-W -S -I"
SPL_AMP="0.25"

SILENT_TIME="0.075"
SILENT_FILE="${FILE_PREFIX}s${FILE_SURFIX}"

gentonefile(){ #by pvfs included in megetty+sendfax ports on FreeBSD.
    NAME=$1; #Tone Name (1,2,3,4..0,*(e),#(f))
    LOW=$2;  #Hz
    HIGH=$3; #Hz
    LOWFILE="${FILE_TMP_PREFIX}.1"
    HIGHFILE="${FILE_TMP_PREFIX}.2"
    TONEFILE="${FILE_PREFIX}${NAME}${FILE_SURFIX}"

#DB#    echo $1 $2 $3
    echo generate $TONEFILE
    pvfsine -F $LOW  -L $SPL_TIME $LOWFILE
    pvfsine -F $HIGH -L $SPL_TIME $HIGHFILE
#    pvfmix -N $LOWFILE $HIGHFILE ${TONEFILE}.pvf
    pvfmix -N $LOWFILE $HIGHFILE ${TONEFILE}
    pvfamp -A${SPL_AMP} ${TONEFILE} ${TONEFILE}.pvf
    rm $LOWFILE $HIGHFILE ${TONEFILE}
    pvftolin $SPL_TYPE ${TONEFILE}.pvf ${TONEFILE}
    rm ${TONEFILE}.pvf
}

array(){
    BUF=$1;
    IDX=$2;
    set -- $BUF;
    while [ $IDX -gt 0 ];do
	shift;
	IDX=`expr $IDX - 1`;
    done
    RET=$1;
}

TNAME="1    2    3    4    5    6    7    8    9    e    0    f"
TLOW=" 697  697  697  770  770  770  852  852  852  941  941  941"
THIGH="1209 1336 1447 1209 1336 1447 1209 1336 1447 1209 1336 1447"
TNUM="12"

TCNT=0;
while [ $TCNT -lt $TNUM ];do
    array "$TNAME" $TCNT
    A=$RET;
    array "$TLOW" $TCNT
    B=$RET;
    array "$THIGH" $TCNT
    C=$RET
    gentonefile $A $B $C
    TCNT=`expr $TCNT + 1`
done


#generate silent
echo "generate ${SILENT_FILE} (for silent)"
pvfsine -L ${SILENT_TIME} "${FILE_TMP_PREFIX}.1"
pvfamp -A 0.0 "${FILE_TMP_PREFIX}.1" "${FILE_TMP_PREFIX}.2"
pvftolin $SPL_TYPE "${FILE_TMP_PREFIX}.2" ${SILENT_FILE}
rm "${FILE_TMP_PREFIX}.1" "${FILE_TMP_PREFIX}.2"
echo "all DONE."