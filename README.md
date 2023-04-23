# FreeSWITCH ASR 模块

**最近很多人都对FreeSWITCH和ASR对接比较感谢兴趣，我之前已经做了一个商业模块（商业模块请点击这里[http://www.ddrj.com/callcenter/asr.html](http://www.ddrj.com/callcenter/asr.html "http://www.ddrj.com/callcenter/asr.html")），考虑到大部分人，只是研究一下，并不准确购买商业模块，特意做一个开源项目给大家提供一个参考。**

mod_asr.cpp 第二个版本，使用了顶顶通VAD（支持噪音人声识别）本程序包的授权文件是10并发1个月的体验授权，仅用于体验和测试使用，商业使用请联系 顶顶通购买正式授权 联系方式 微信 cdevelop 网站 www.ddrj.com

![](wx.png)

## 顶顶通VAD介绍

语音活动检测(Voice Activity Detection,VAD)，就是检测是否有声音，常规的算法是通过声音音量和频谱特诊来判断是否有声音的，但是无法区分是噪音还是人声，在电话机器人中**噪音打断**和**噪音识别错误的关键词**始终是一个痛点，机器学习算法可以通过大量噪音和人声数据训练出判别人声还是噪音的神经网络模型，VAD算法结合深度神经网络就可以彻底解决这个痛点了。 顶顶通的最新VAD算法已经集成了人声噪音判别引擎。

### 噪音识别的用处

- 防止错误的意向判断

  噪音识别成关键词（是，恩，哦），导致把无意向客户判断成有意向客户，通过噪音识别模块，过滤掉噪音，可以大大提高机器人的意向判断准确度。

- 防止噪音打断机器人说话
  大部分机器人只要开了打断功能，有一点噪音就给错误打断了，根本没法在生产环境开打断功能，有了噪音识别模块，就可以避免噪音打断了。

- 机器人反应更灵敏

  噪音环境VAD无法判断用户说话结束，会导致用户说话完成了，机器人还一直傻等，有了噪音识别模块，可以让机器人反应更加灵敏。

- 节约ASR费用

  在电话机器人业务中，大量的无效声音(各种噪音)调用ASR，浪费ASR调用费用，有了噪音人声判别功能，就可以噪音不再调用ASR接口，节约大量ASR费用。

### 噪音人声识别算法原理

基于20G的噪音声音文件和100G的正常人声的声音文件，使用tdnn(时延神经网络)和 lstm(长短期记忆网络)训练出噪音人声音判别模型。

### 噪音人声识别的准确率

准确率取决训练数据的准确性，目前的模型大于1秒声音准确率大于99.9%， 300毫秒以内短时人声和质量很差的人声，有少量识别成噪音的错误率，因为噪音库包含了大量的背景人声。



## **2023-2-28 第二版本代码提交**
请在FreeSWITCH 1.8以上版本测试，低于1.8版本需要修改代码：switch_buffer.c没有switch_buffer_get_head_pointer这个函数。
- 安装 libsad 

  - 目录 copy到 /var目录，最后的路径是
    - 授权文件 /var/libsad/license.jon 
    - 模型目录 /var/libsad/model/ 
    - lib文件 /var/libsad/libsad.so
  - mod_asr.so copy到 fs的mod目录
  - fs_cli 执行 load mod_asr 加载模块。

- 申请ASR  本例子使用多方asr接口，注册地址 http://ai.hiszy.com/#/user/register?code=RK9RD7W 注册后每天可以免费测试1000次。需要更多次数可以 联系微信 aohu6789 购买，请先说明是顶顶通开源接口，充值有优惠。

  在fs安装目录/etc/vars.xml  配置asr key

  ```
    <X-PRE-PROCESS cmd="set" data="appKey=asr后台的appkey"></X-PRE-PROCESS>
    <X-PRE-PROCESS cmd="set" data="appSecret=asr后台的appSecret"></X-PRE-PROCESS>
    
  ```


- 测试

  执行动作 play_and_asr  参数 playfilename waittime maxspeaktime allowbreak recordfilename
  语音识别结果存入通道变量asr_result，如果没有检测到声音设置为silence
  - playfilename   放音文件
  - waittime  等待说话时间，放音完成开始计算
  - maxspeaktime  最大说话时间
  - allowbreak  是否允许打断，检测到说话就停止放音
  - recordfilename  本次说话录音文件

   例子
   ```
    <action application="play_and_asr" data="welcome.wav  5000 10000 true /tmp/speak.wav"/>
    <action application="log" data="open=${asr_result}"/>
   ```


- 编译
如果修改了代码编译方法是 
g++ -shared -fPIC -o mod_asr.so mod_asr.cpp -I /usr/local/freeswitch/include/freeswitch -L /usr/local/freeswitch/lib -lfreeswitch -L /var/libsad/ -lsad -Wl,-rpath=/var/libsad




- 兼容第一版本的测试方法，asr key 在 vars.xml 里面设置。

			<extension name="asr">
				<condition field="destination_number" expression="^(888)$">
					<action application="answer"/>
					<action application="start_asr"/>
					<action application="park"/>
				</condition>
			</extension> 




mod_asr_aliyun.cpp 第一个版本实现 [阿里云ASR](https://help.aliyun.com/document_detail/30416.html?spm=5176.doc30416.3.2.PuOAsM "阿里云ASR")和FreeSWITCH的直接对接，把识别结果通过ESL输出。

## **2017-12-10 第一版本代码提交**

- 安装
	- 如果你觉得自己编译太麻烦，可以直接下载我编译好的，放在bin 目录里面。路径请根据自己情况修改。**注意只支持x64系统**
	- mod_asr.so 复制到到 FreeSWITCH mod 目录。 `cp mod_asr.so /usr/local/freeswitch/mod/`,如果是fs1.2,请用fs1.2_mod_asr.so的那个。
	- librealTimeUnity.so FreeSWITCH lib 目录，或者系统lib目录。`cp librealTimeUnity.so /usr/local/freeswitch/lib/`
	- libopus.so FreeSWITCH lib 目录，或者系统lib目录。`cp libopus.so.0 /usr/local/freeswitch/lib/`
	- config-realtime.txt 复制到 /etc/目录。`cp config-realtime.txt /etc/`
	- /usr/local/freeswitch/conf/autoload_configs/modules.conf.xml 加入 `<load module="mod_asr"/>`
	- 重启FreeSWITCH，或者fs_cli 里面执行 `reload mod_asr`
	- **注意**默认只支持单声道8000hz的编码,opus或者g722编码不支持。


- 编译
	- 下载阿里云语音识别SDK [http://download.taobaocdn.com/freedom/33762/compress/nlsSpeech-release.zip?spm=5176.doc48715.2.4.bRraen&file=nlsSpeech-release.zip](http://download.taobaocdn.com/freedom/33762/compress/nlsSpeech-release.zip?spm=5176.doc48715.2.4.bRraen&file=nlsSpeech-release.zip "http://download.taobaocdn.com/freedom/33762/compress/nlsSpeech-release.zip?spm=5176.doc48715.2.4.bRraen&file=nlsSpeech-release.zip")
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
	
