# tunproxy
**tunproxy** is a software dedicated to route UDP traffic to SOCKS5 proxy server.
It is designed for Linux operating system.

To compile program simply write\
`cmake .`\
and then\
`make`

You can run like this\
`./tunproxy 192.168.0.151 1080`\
or\
`./tunproxy 192.168.0.151:1080`\

**tunproxy** also has logging capability, simply add **-l** option and provide filename
`./tunproxy 192.168.0.151:1080 -l /tmp/log.txt`


