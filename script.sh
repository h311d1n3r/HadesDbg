for i in $(seq 1 1 `ls ./test/bin | wc -l`)
do
  echo "Starting test n°$i"
  hadesdbg ./test/bin/test_$i -script ./test/scripts/test_$i.hscript -output ./test_$i.hout -entry 0x1040 -bp 0x1131:15
  if [[ $(md5sum ./test_$i.hout | cut -d' ' -f1) == $(md5sum ./test/results/test_$i.hout | cut -d' ' -f1) ]]
  then
    echo "Success !"
  else
    echo "Failure !"
    echo "Content of result file :"
    cat ./test_$i.hout
    exit 1
  fi
done
