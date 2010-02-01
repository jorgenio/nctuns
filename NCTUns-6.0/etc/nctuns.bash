export NCTUNSHOME=/usr/local/nctuns
export NCTUNS_TOOLS=$NCTUNSHOME/tools
export NCTUNS_BIN=$NCTUNSHOME/bin

IFS=:
for list in $PATH
do
    if [ "`echo $list | awk '($1 ~ /nctuns/) {print "ok"}'`" == "ok" ]; then
        unset IFS
        return;
    fi
done
unset IFS
export PATH=${NCTUNS_BIN}:${PATH}
