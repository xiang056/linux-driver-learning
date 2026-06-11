#!/bin/bash
# 用法：sudo bash test.sh   （載入內核模組需要 root）
set -e
cd "$(dirname "$0")"

echo "############ 1) 預設值載入 ############"
insmod ./hello_param.ko
dmesg | tail -3
rmmod hello_param
echo

echo "############ 2) 傳入參數載入 ############"
echo '   insmod ./hello_param.ko param_value=100 name="World" arr=1,2,3'
insmod ./hello_param.ko param_value=100 name="World" arr=1,2,3
dmesg | tail -6
echo

echo "############ 3) 看 sysfs 裡的參數（perm=0644 才會出現）############"
for f in /sys/module/hello_param/parameters/*; do
    echo "   $f = $(cat "$f")"
done
echo

echo "############ 4) 執行期改參數（demo 0644 可寫）############"
echo 999 > /sys/module/hello_param/parameters/param_value
echo "   改完 param_value = $(cat /sys/module/hello_param/parameters/param_value)"
echo "   (註：改 sysfs 只改記憶體裡的值，卸載時 exit 會印出最新值)"
echo

echo "############ 5) 卸載，看 exit 印出的最終值 ############"
rmmod hello_param
dmesg | tail -2
echo
echo "DONE ✅"
