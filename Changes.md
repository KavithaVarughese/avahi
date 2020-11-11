<h2> Installation </h2>

Ubuntu 18.04 or more

<strong> Dependencies Installed </strong> <br>
```
sudo apt-get install libtool automake expat libgdbm-dev libdaemon-dev gtk+2.0 doxygen xmltoman qt5-default
sudo apt-get install -y m4 glib2.0 dbus glade
sudo apt install pkg-config libevent-dev
```

<strong> Additional Files Required in the root directory </strong> <br>
1. /hex_packet_verbose.txt - hexdump of the mdns field of the generated packets <br>
```
sudo touch /hex_packet_verbose.txt && sudo chmod 777 /hex_packet_verbose.txt
```
Note: Is never automatically emptied <br>

2. /hex_packet.txt - hex string of the mdns field of the generated packets in new lines <br>
```
sudo touch /hex_packet.txt && sudo chmod 777 /hex_packet.txt
```
Note: Is never automatically emptied <br>

3. /browse_service.csv - file containing semi colon seperated values for each customised packets in new lines <br>
```
sudo touch /browse_service.csv && sudo chmod 777 /browse_service.csv
```
A sample file is present in the repository <br>
Note: The file is emptied regularly when the customised packets for the respective rows are created.

<strong> Installation </strong> <br>
```
git clone https://github.com/KavithaVarughese/avahi.git -b browse Avahi-Browse
cd Avahi-Browse
sudo ./autogen.sh --disable-python
sudo ./configure --sysconfdir=/etc --localstatedir=/var --disable-python
sudo make
sudo make install
sudo ldconfig
```
If the below groups dont already exist: <br>
```
sudo addgroup --system avahi
sudo adduser --system --no-create-home --ingroup avahi avahi
```

<hr>

## Run
```
sudo ./start.sh
```
or <br>
```
sudo bash start.sh
```

<h2> Changes </h2>

<h3> New files added </h3>
1. avahi-core/customised-packets.h<br>
2. avahi-core/customised-packets.c<br>
3. avahi-daemon/dbus-print-message.h<br>
4. avahi-daemon/dbus-print-message.c<br>

<h3> Relevent changes in files </h3>

1. start.sh
  - Complete file
2. avahi-core/Makefile.am
- line 63 : header for customised packets
3. avahi-core/customised-packets.c and avahi-core/customised-packets.h
- Complete file
4. avahi-core/util.c
- lines 33 to 100
5. avahi-core/query-sched.c
- lines 297 to 300
6. avahi-daemon/Makefile.am
- line 56
7. avahi-daemon/dbus-print-message.c
- Complete file : Can be used to print DbusMessage for logs<br>
All other changes are for logs
