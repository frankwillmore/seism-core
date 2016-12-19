
function processor_geometry {

grep "^$1" << EOF | awk '{print $2}' | sed -e "s/:/ /g" 
8 2:2:2
16 2:2:4
24 2:3:4
36 3:3:4
48 3:4:4
64 4:4:4
96 4:4:6
384 6:8:8
768 8:8:12
1536 8:12:16
3072 12:16:16
EOF

}

