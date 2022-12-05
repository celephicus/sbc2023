# Mods to Relay Schematic from Checkout Nov 2022

1. Change U2 label to 3.3V
2. U2 footprint pins 2,3 swapped, schema OK.
3. R10 under J9.
4. Connect R13/R14 jn to M1/D2 (ATN)
5. Tie M1/A6 to 0V.
6. Swap labels CONSOLE_TX & CONSOLE_RX on processor only.
7. Need PTC in BUS_VCC.
8. Rename links A1, A2 with little graphic to note IDs. A1 open; A2 open: 1, A1 open; A2 closed: 2, etc. 
9. J6,J7 should have one select & 1 ground per link. Have to bend pins to fit links. 
10. Change value R6 to 3K3. 
11. Add link for bus termination. 
12. Enlarge D2 holes to 1.0mm.


