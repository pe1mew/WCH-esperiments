# WCH MCU documentation on the web

 - https://github.com/limingjie/WCH-MCU-Pinouts/tree/main


# Source downloaded PDF

 - https://www.olimex.com/Products/RISC-V/WCH/WCH-LinkE/resources/WCH-LinkUserManual.PDF
 
# Wiring WCH-Link to nanoCH32V003

```

   +--------------+    +----------------+
   | nanoCH32V003 |    |       WCH-Link |
   |          GND o----o GND            |
   |          DI0 o----o SWDIO/TMS      |
   |          VCC o----o 3V3            |
   |              |    |                |
   +--------------+    +----------------+