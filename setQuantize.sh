#!bash
#setQuantize <camera> <factor>
x="[[0x44,$2]]"
curl -H 'Content-Type: application/json' \
            -X PUT http://192.168.0.155:7777/cam/$1/pgm \
            -d "{\"pgm\": [ [\"0x44\", \"0x0A\"] ]}"
