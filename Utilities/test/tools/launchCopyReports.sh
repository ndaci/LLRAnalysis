ls $1/*/res/lumiSummary.json | grep "Data" | awk -v dir=$1 -F "/" '{print "cp "$dir"/"$1"/"$2"/"$3" /data_CMS/cms/htautau/JSON/report/report_"$1".txt"}' > copyReports.sh ; chmod u+x copyReports.sh ; ./copyReports.sh ; rm copyReports.sh