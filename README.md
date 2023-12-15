# macropad-ESP32
## PCB design 
![](https://github.com/WiktorK02/macropad-ESP32/assets/123249470/5bded6bd-c70d-425e-8246-885c9c012832)
![](https://github.com/WiktorK02/macropad-ESP32/assets/123249470/cac6983b-8be3-43c5-9341-99c6b382dec0)
## MQTT Doc
### Topics
<b>"con/ok"</b> -> while connection to mqtt and wifi returns TRUE,<br>
<b>"analog/read" </b>-> read rotary encoder value,<br>
<b>"button/click" </b>-> rotary encoder single click,<br>
<b>"switch/click/1" </b>-> first switch is clicked,<br>
<b>"switch/click/2" </b>-> second switch is clicked
## Components 
 - ESP32
 - Cherry MX Red switches
 - 10k resistors
 - Encoder 3.3V
## What is done
 - Sending MQTT messages.
 - Managing WiFi and MQTT setup and saving them to a JSON file.
 - Pull up resistors with switches have been checked.
 - Proper saving settings to JSON
 - More stable
## Task list
 - <del>Testing pull-up resistors with switches.<del>
 - <del>Debugging the issue with saving WiFi and MQTT settings during reset.<del>
 - <del>Final test with every componet<del>
 - <del>Buy PCB<del>
 - Reset settings button
 - Remove useless variables/clean the code
 - Remove spikes/hazards from rotary encoder signal(capacitors do not work)
## How to Contribute
1. Fork the Project
2. Clone repo with your GitHub username instead of ```YOUR-USERNAME```:<br>
```
$ git clone https://github.com/YOUR-USERNAME/macropad-ESP32
```
3. Create new branch:<br>
```
$ git branch BRANCH-NAME 
$ git checkout BRANCH-NAME
```
4. Make changes and test<br>
5. Submit Pull Request with comprehensive description of change
## License 
[MIT license](LICENSE)
