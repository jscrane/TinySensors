- reconnecting lcd guy (need to reinitialise lcdproc client too)
- sensor for external weather and chill
- add mux client for mqtt (https://opensensors.io/)
- configure tinysensors not to retransmit if hub down
- rebuild rf24-rpi lib with external bcm library (check
  very different spi_begin() implementations)
