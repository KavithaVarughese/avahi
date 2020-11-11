<h2> Installation </h2>

Ubuntu 18.04 or more

<strong> Dependencies Installed </strong> <br>
`sudo apt-get install libtool automake expat libgdbm-dev libdaemon-dev gtk+2.0 doxygen xmltoman qt5-default`
`sudo apt-get install -y m4 glib2.0 dbus glade`
`sudo apt install pkg-config libevent-dev`

<strong> Additional Files Required in the root directory </strong> <br>
1. /hex_packet_verbose.txt - hexdump of the mdns field of the generated packets <br>
sudo touch /hex_packet_verbose.txt && sudo chmod 777 /hex_packet_verbose.txt <br>
Note: Is never automatically emptied <br>
2. /hex_packet.txt - hex string of the mdns field of the generated packets in new lines <br>
sudo touch /hex_packet.txt && sudo chmod 777 /hex_packet.txt <br>
Note: Is never automatically emptied <br>
3. /ip.csv - file containing semi colon seperated values for each customised packets in new lines <br>
sudo touch /ip.csv && sudo chmod 777 /ip.csv <br>
A sample file is present in the repository <br>
Note: The file is emptied regularly when the customised packets for the respective rows are created.

<strong> Installation </strong> <br>
git clone https://github.com/KavithaVarughese/avahi.git -b main Avahi-Main <br>
cd Avahi-Main <br>
sudo ./autogen.sh --disable-python <br>
sudo ./configure --sysconfdir=/etc --localstatedir=/var --disable-python <br>
sudo make <br>
sudo make install <br>
sudo ldconfig <br>

If the below groups dont already exist: <br>
sudo addgroup --system avahi <br>
sudo adduser --system --no-create-home --ingroup avahi avahi <br>

<hr>

<h2> Run </h2> <br>
sudo ./start.sh <br>
or <br>
sudo bash start.sh <br>

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
- line 62 : header for customised packets
3. avahi-core/customised-packets.c and avahi-core/customised-packets.h
- Complete file
4. avahi-core/util.c
- lines 33 to 104
5 avahi-core/response-sched.c
- lines 91 to 174
--Note : Commented lines 154 to 170 is for infinite loop within the program, else start.sh consists of the infinite loop
- lines 373 to 379 
6. avahi-daemon/Makefile.am
- line 56
7. avahi-daemon/dbus-print-message.c
- Complete file : Can be used to print DbusMessage for logs<br>
All other changes are for logs
