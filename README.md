# FreeSWITCH ASR 模块

**最近很多人都对FreeSWITCH和ASR对接比较感谢兴趣，我之前已经做了一个商业模块（商业模块请点击这里[http://www.dingdingtong.cn/smartivr/](http://www.dingdingtong.cn/smartivr/ "http://www.dingdingtong.cn/smartivr/")），考虑到大部分人，只是研究一下，并不准确购买商业模块，特意做一个开源项目给大家提供一个参考。**

第一个版本实现 [阿里云ASR](https://help.aliyun.com/document_detail/30416.html?spm=5176.doc30416.3.2.PuOAsM "阿里云ASR")和FreeSWITCH的直接对接，把识别结果通过ESL输出。


想了开发进度，和本项目的技术交流欢迎加QQ群：340129771

![](qq_qun.png)



**2017-12-10 第一版本代码提交**

- 安装
	- 如果你觉得自己编译太麻烦，可以直接下载我编译好的，放在bin 目录里面。路径请根据自己情况修改。**注意只支持x64系统**
	- mod_asr.so 复制到到 FreeSWITCH mod 目录。 `cp mod_asr.so /usr/local/freeswitch/mod/`
	- librealTimeUnity.so FreeSWITCH lib 目录，或者系统lib目录。`cp librealTimeUnity.so /usr/local/freeswitch/lib/`
	- libopus.so FreeSWITCH lib 目录，或者系统lib目录。`cp libopus.so.0 /usr/local/freeswitch/lib/`
	- config-realtime.txt 复制到 /etc/目录。`cp config-realtime.txt /etc/`
	- /usr/local/freeswitch/conf/autoload_configs/modules.conf.xml 加入 `<load module="mod_asr"/>`
	- 重启FreeSWITCH，或者fs_cli 里面执行 `reload mod_asr`


- 编译
	- 下载阿里云语音识别SDK [http://download.taobaocdn.com/freedom/33762/compress/RealtimeDemo.zip?spm=5176.doc35312.2.4.Yh8Rr0&file=RealtimeDemo.zip](http://download.taobaocdn.com/freedom/33762/compress/RealtimeDemo.zip?spm=5176.doc35312.2.4.Yh8Rr0&file=RealtimeDemo.zip "http://download.taobaocdn.com/freedom/33762/compress/RealtimeDemo.zip?spm=5176.doc35312.2.4.Yh8Rr0&file=RealtimeDemo.zip")
	- 安装FreeSWITCH [https://freeswitch.org/confluence/display/FREESWITCH/Installation](https://freeswitch.org/confluence/display/FREESWITCH/Installation "https://freeswitch.org/confluence/display/FREESWITCH/Installation")
	- 下载mod_asr代码 `git clone https://github.com/cdevelop/FreeSWITCH-ASR.git`
	- 编译 `g++ -shared -fPIC -o mod_asr.so mod_asr.cpp -pthread -I ./nlsSpeech-release/include -I /usr/local/freeswitch/include -L./nlsSpeech-release/lib/linux -L /usr/local/freeswitch/lib -ldl -lopus -lrealTimeUnity -lfreeswitch`，nlsSpeech-release和FreeSWITCH路径请根据自己情况修改。

- 使用
	- 申请阿里云的Access Key ID 和 Secret ，请参考 [https://help.aliyun.com/document_detail/30437.html?spm=5176.doc35312.6.539.7eNuaN](https://help.aliyun.com/document_detail/30437.html?spm=5176.doc35312.6.539.7eNuaN "https://help.aliyun.com/document_detail/30437.html?spm=5176.doc35312.6.539.7eNuaN")，如果你还没有，可以先使用我已经申请的id（LTAIRLpr2pJFjQbY）和key（oxrJhiBZB5zLX7LKYqETC8PC8ulwh0）测试。
	- fs_cli 执行  **originate user/1001 'start_asr:id secret,park' inline**，如 `bgapi originate user/1001 'start_asr:LTAIRLpr2pJFjQbY oxrJhiBZB5zLX7LKYqETC8PC8ulwh0,park' inline`，分机接起来开始说话，就可以看到识别结果输出了。 输出结果的日志等级是  notify  `console loglevel 5`
	- dialplan中使用 **测试的时候如果不执行其他APP，park超时会自动挂断，可以加入`<action application="set" data="park_timeout=60"/>`修改park超时时间。**
	
			<extension name="asr">
				<condition field="destination_number" expression="^(888)$">
					<action application="answer"/>
					<action application="start_asr" data="LTAIRLpr2pJFjQbY oxrJhiBZB5zLX7LKYqETC8PC8ulwh0"/>
					<action application="park"/>
				</condition>
			</extension> 

- 开发
	- fs_cli 测试方式 ，执行 `/event custom asr` 订阅事件。
	- 识别结果会通过esl输出（需要订阅 `custom asr` 事件）（阿里云返回的原始json数据）例子如下：
		
			RECV EVENT
			Event-Subclass: asr
			Event-Name: CUSTOM
			Core-UUID: 48a08a69-7858-407a-be69-679150d34193
			FreeSWITCH-Hostname: MiWiFi-R3D-srv
			FreeSWITCH-Switchname: MiWiFi-R3D-srv
			FreeSWITCH-IPv4: 192.168.31.164
			FreeSWITCH-IPv6: ::1
			Event-Date-Local: 2017-12-10 11:30:32
			Event-Date-GMT: Sun, 10 Dec 2017 03:30:32 GMT
			Event-Date-Timestamp: 1512876632835590
			Event-Calling-File: mod_asr.cpp
			Event-Calling-Function: OnResultDataRecved
			Event-Calling-Line-Number: 55
			Event-Sequence: 914
			ASR-Response: {"finish":0,"request_id":"ee87d7fd5e304bdaa9343d9262f34125","result":{"sentence_id":2,"begin_time":4200,"end_time":6525,"status_code":0,"text":"美国拜拜"},"status_code":200,"version":"2.0"}
			Channel: sofia/external/linphone@192.168.31.210
		
		ASR-Response：asr返回结果。
		Channel:当前通道。主要使用这2个通道变量。
	- 如果你需要用户说完一整句话，再一次性返回结果。请把`config-realtime.txt`文件`ResponseMode:streaming`修改为`ResponseMode:normal`。
	- 如果你觉得自己开发太麻烦了，可以联系QQ：1280791187 或者微信：cdevelop，获取商业服务和支持。
	