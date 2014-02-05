OUTPUT1=$(./ffTES ./tx.mp3)
OUTPUT2=$(./ffTES ./tx.flac)

OUTPUT_ARRRAY1=($OUTPUT1)
OUTPUT_ARRRAY2=($OUTPUT2)

HAMMING_DIST=$(./demo ${OUTPUT_ARRRAY1[1]} ${OUTPUT_ARRRAY2[1]} ${OUTPUT_ARRRAY1[0]})

echo "signature1:" ${OUTPUT_ARRRAY1[1]}
echo "signature2:" ${OUTPUT_ARRRAY2[1]}
echo "Normalized hamming distance is:" $HAMMING_DIST

