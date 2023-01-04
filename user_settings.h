// the following options while defaulted to here can also be changed via the program's settings window:

#define DISPLAY_TIME_AND_DATE                   0                           // Display time and date options    
                                                                            // 0: Display the time and date
                                                                            // 1: Display the time only
											 							                                        // 2: Display the date only 
																		   																      
#define DISPLAY_TIME_IN_TWELVE_HOUR_FORMAT      true                        // if set to true for 12 hour time format, set to false for 24 hour time format

#define DISPLAY_TIME_WITH_SECONDS               false                       // if set to true display seconds, it set to false don't display the seconds
																		   																      													
#define DISPLAY_AM_PM                           true                        // if DISPLAY_TIME_IN_TWELVE_HOUR_FORMAT is true, then a AM/PM indicator will be displayed if true otherwise a AM/PM indicator will not be displayed
                                                                            
#define DISPLAY_TIME_WITH_10THS_OF_A_SECOND     false                       // if set to true display 10th of a second, it set to false don't display 10th of a second	
																		   																      	
#define DATE_FORMAT                             2                           // Date formats:
                                                                            //   0: YYYY-MM-DD
                                                                            //   1: YYYY/MM/DD
                                                                            //   2: YY-MM-DD
                                                                            //   3: YY/MM/DD
                                                                            //   4: MM-DD-YY
                                                                            //   5: MM/DD/YY
                                                                            //   6: MM-DD
                                                                            //   7: MM/DD
                                                                            //   8: DD-MM
                                                                            //   9: DD/MM
																		   																      	
#define BACKGROUND_FORMAT                      0                            // Background formats:															     
                                                                            //   0: black background
                                                                            //   1: white background
                                                                            //   2: unique background colour for every second
                                                                            //   3: unique background colour each second		   																      	

#define WIFI_CONNECTING_STATUS_COLOUR          TFT_GREEN                    // the text colour of the display during WIFI connection process

#define MY_TIME_ZONE                          "EST5EDT,M3.2.0,M11.1.0"      // Time Zone for America/Toronto   
                                                                            // Supported Timezones are listed here: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
                                                                            //
#define PRIMARY_TIME_SERVER                   "time.nrc.ca"                 // primary ntp time server
#define SECONDARY_TIME_SERVER                 "ca.pool.ntp.org"             // secondary ntp time server
#define TERTIARY_TIME_SERVER                  "north-america.pool.ntp.org"  // tertiary ntp time server 
                                                                            // alternative ntp time servers / server pools:                                                                          
                                                                            //    "time.nrc.ca"                        // Ottawa, Ontario, Canada
                                                                            //    "ca.pool.ntp.org"                    // Canada
                                                                            //    "asia.pool.ntp.org"
                                                                            //    "europe.pool.ntp.org"
                                                                            //    "north-america.pool.ntp.org" 
                                                                            //    "oceania.pool.ntp.org"
                                                                            //    "south-america.pool.ntp.org" 
                                                                            //    "pool.ntp.org"                       // World wide      

#define SERIAL_MONITOR_SPEED                  115200                        // serial monitor speed

#define DEBUG_IS_ON                           false                          // show debug info to Serial monitor windows

#define ALLOW_ACCESS_TO_NETWORK               true                          // used for testing, set to false to disallow access to your Wifi network

#define ALLOW_ACCESS_TO_THE_TIME_SERVER       true                          // used for testing, set to false to disallow access to the time server

#define SHOW_LEADING_ZERO_TUBE_STYLE          0                             // 0: Show a leading zero as a zero tube
                                                                            // 1: Show a leading zero as a blank tube
                                                                            // 2: Don't show a leading zero

#define BACKGROUND_COLOUR_ROTATION_METHOD     1                             // 0: start at a random colour and then progressively rotate through all feasable colours each time this routine is called
                                                                            //    increasing red, then green, then blue each in turn by one
                                                                            //
                                                                            // 1: start at a random colour and then progressively rotate through all feasable colours each time this routine is called
                                                                            //    by increasing red from 0 to 31, then within that green from 0 to 63, then within that blue from 0 to 31
                                                                            //
                                                                            // 2: each second of the day will have a unique colour associated with it
                                                                            //    red will be used for hours, blue will be used for minutes, green will be used for seconds
                                                                            //