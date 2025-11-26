loongarch64-linux-gnu-objdump -d -M no-aliases "$1" | awk '{ print $3 }' | sort -n | uniq -c | sort -h
