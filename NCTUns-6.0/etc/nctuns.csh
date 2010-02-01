setenv NCTUNSHOME /usr/local/nctuns
setenv NCTUNS_TOOLS $NCTUNSHOME/tools
setenv NCTUNS_BIN $NCTUNSHOME/bin

set LIST=`echo $PATH | cut -d : --output-delimiter " " -f -`
foreach list ( $LIST )
    if ( `echo $list | awk '($1 ~ /nctuns/) {print "ok"}'` == "ok" ) then
        exit 0
    endif
end
setenv PATH ${NCTUNS_BIN}:${PATH}
