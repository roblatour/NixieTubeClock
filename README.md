# NixieTubeClock
Nixie tube clock (for an ESP32 driving a TFT display)
Rob Latour, 2023

## Features

- Shows a Nixie Tube clock
- Many settings, include those for Date and Time formats are easily customizable and stored in non-volatile memory
- Date and Time are set and refreshed automatically via NTP server pools
- ESP32 is only connected to the network when being setup and for NTP time updates
- Network SSID and Password are not hardcoded in the sketch, rather program allows you to enter these via you computer or phone's default browser
- uses open source NixieTube library ( https://github.com/roblatour/NixieTubes ) 

![image](https://user-images.githubusercontent.com/5200730/210621274-969581ec-2961-4eeb-93b3-a9b1acfaef04.png)

## License

MIT

## Support

Want to contribute? Great!

Please visit https://rlatour.com 
