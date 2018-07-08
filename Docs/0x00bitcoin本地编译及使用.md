# 0x00bitcoin本地编译及使用


# 前言

入坑区块链，对于第一个把区块链技术推向巅峰的比特币不能不了解。而了解一项技术应用的最佳途径莫过于亲自编译和阅读比特币的源码，尽管笔者可能力有不逮，但愿意一试。我想，过程可能很艰难，终点会有美丽的风景等着我。

推荐[精通比特币第二版](http://book.8btc.com/books/6/masterbitcoin2cn/_book/)，讲解比特币原理非常不错的一本书。

# [bitcoin](https://github.com/bitcoin) github项目介绍

打开bitcoin的github地址，我们发现共有四个目录：

- bitcoin 比特币核心源代码 C++

- bips 有关比特币改进建议的相关文档  我们经常听到的“”

- libbase58  比特币base58编解码库  C

- libblkmaker  比特币区块模板库 C

![bitcoin项目目录](https://upload-images.jianshu.io/upload_images/830585-cce029da493ad92f.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

打开bitcoin,我们看一下核心代码的架构：

![bitcoin项目结构](https://upload-images.jianshu.io/upload_images/830585-17ae77b39f4f2a13.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

![Mac下bitcoin编译文档](https://upload-images.jianshu.io/upload_images/830585-436450310a8e7e66.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

# 撸起袖子就是干

##### 1.从github拉取对应版本的bitcoin源码
```
//拉取bitcoin源代码
git clone https://github.com/bitcoin/bitcoin.git

//切换版本到最新
git checkout 0.16
```

##### 2.按官方build文档依次执行

看官方build文档竟意外地发现，可以用Qt来构建和调试bitcoin。有了IDE之后，在看源码的时候，相关函数就可以直接跳转到定义的地方。不知方便了多少。。。

按着步骤来，一步也不要拉下，不然后面可能会有意向不到的障碍。约摸10min左右，整个build过程结束。

##### 3.几个主要程序

- __Bitcoin Core.app__  Bitcoin客户端图形界面版

- __bitcoind__   /src/bitcoind  Bitcoin简洁命令行版,也是下一步源代码分析的重点(不能与Bitcoin Core同事运行，如果不小心尝试同时运行另外一个客户端，它会提示已经有一个客户端在运行并且自动退出)

- __bitcoin-cli__  /src/bitcoin-cli Bitcoind的一个功能完备的RPC客户端，可以通过它在命令行查询某个区块信息，交易信息等

- __bitcoin-tx__   /src/bitcoind   比特币交易处理模块，可以进行交易的查询和创建

##bitcoind

##### 1.启动与结束bitcoind
```
cd .../bitcoin/src

//1.启动bitcoind,会联网进行block的同步，180多G耗时耗内存
./bitcoind

//2.启动并指定同步数
./bitcoind -maxconnections=0

//3.或者在1启动后通过bitcoin-cli无效化当前区块
./bitcoin-cli invalidateblock `bitcoin-cli getbestblockhash`

//4.结束
./bitcoin-cli stop
```
##### ___注___：如果突然发现你的Mac内存不够用了，老铁，一定是bitcoin干的！Mac下默认的block同步路径可是让我好找，贴出来省得大家到时候懵逼不知道哪里找:
```
/Users/[User]/Library/Application Support/Bitcoin/blocks
```

##### 2. bitcoind其他命令

./bitcoind -help可以查看bitcoind支持的各种命令和带参格式：

![bitcoind -help](https://upload-images.jianshu.io/upload_images/830585-340aa07311dc0aeb.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

```
 //bitcoind 命令通用格式

  bitcoind [选项]

  bitcoind [选项] <命令> [参数]  将命令发送到 -server 或 bitcoind

  bitcoind [选项] help           列出命令

  bitcoind [选项] help <命令>    获取该命令的帮助


 //bitcoind常见命令

  -conf=<文件名>     指定配置文件（默认：bitcoin.conf）

  -pid=<文件名>      指定 pid （进程 ID）文件（默认：bitcoind.pid）

  -gen               生成比特币

  -gen=0             不生成比特币

  -min               启动时最小化

  -splash            启动时显示启动屏幕（默认：1）

  -datadir=<目录名>  指定数据目录

  -dbcache=<n style="word-wrap: break-word;">       设置数据库缓存大小，单位为兆字节（MB）（默认：25）</n>

  -dblogsize=<n style="word-wrap: break-word;">     设置数据库磁盘日志大小，单位为兆字节（MB）（默认：100）</n>

  -timeout=<n style="word-wrap: break-word;">       设置连接超时，单位为毫秒</n>

  -proxy=<ip:端口 style="word-wrap: break-word;">   通过 Socks4 代理链接</ip:端口>

  -dns               addnode 允许查询 DNS 并连接

  -port=<端口>       监听 <端口> 上的连接（默认：8333，测试网络 testnet：18333）

  -maxconnections=<n style="word-wrap: break-word;">  最多维护 <n style="word-wrap: break-word;">个节点连接（默认：125）</n></n>

  -addnode=<ip style="word-wrap: break-word;">      添加一个节点以供连接，并尝试保持与该节点的连接</ip>

  -connect=<ip style="word-wrap: break-word;">      仅连接到这里指定的节点</ip>

  -irc               使用 IRC（因特网中继聊天）查找节点（默认：0）

  -listen            接受来自外部的连接（默认：1）

  -dnsseed           使用 DNS 查找节点（默认：1）

  -banscore=<n style="word-wrap: break-word;">      与行为异常节点断开连接的临界值（默认：100）</n>

  -bantime=<n style="word-wrap: break-word;">       重新允许行为异常节点连接所间隔的秒数（默认：86400）</n>

  -maxreceivebuffer=<n style="word-wrap: break-word;">  最大每连接接收缓存，<n style="word-wrap: break-word;">*1000 字节（默认：10000）</n></n>

  -maxsendbuffer=<n style="word-wrap: break-word;">  最大每连接发送缓存，<n style="word-wrap: break-word;">*1000 字节（默认：10000）</n></n>

  -upnp              使用全局即插即用（UPNP）映射监听端口（默认：0）

  -detachdb          分离货币块和地址数据库。会增加客户端关闭时间（默认：0）

  -paytxfee=<amt style="word-wrap: break-word;">    您发送的交易每 KB 字节的手续费</amt>

  -testnet           使用测试网络

  -debug             输出额外的调试信息

  -logtimestamps     调试信息前添加[时间戳](http://8btc.com/article-165-1.html)

  -printtoconsole    发送跟踪/调试信息到控制台而不是 debug.log 文件

  -printtodebugger   发送跟踪/调试信息到调试器

  -rpcuser=<用户名>  JSON-RPC 连接使用的用户名

  -rpcpassword=<密码>  JSON-RPC 连接使用的密码

  -rpcport=<port style="word-wrap: break-word;">    JSON-RPC 连接所监听的 <端口>（默认：8332）</port>

  -rpcallowip=<ip style="word-wrap: break-word;">   允许来自指定 <ip style="word-wrap: break-word;">地址的 JSON-RPC 连接</ip></ip>

  -rpcconnect=<ip style="word-wrap: break-word;">   发送命令到运行在 <ip style="word-wrap: break-word;">地址的节点（默认：127.0.0.1）</ip></ip>

  -blocknotify=<命令> 当最好的货币块改变时执行命令（命令中的 %s 会被替换为货币块哈希值）

  -upgradewallet     将钱包升级到最新的格式

  -keypool=<n style="word-wrap: break-word;">       将密匙池的尺寸设置为 <n style="word-wrap: break-word;">（默认：100）</n></n>

  -rescan            重新扫描货币块链以查找钱包丢失的交易

  -checkblocks=<n style="word-wrap: break-word;">   启动时检查多少货币块（默认：2500，0 表示全部）</n>

  -checklevel=<n style="word-wrap: break-word;">    货币块验证的级别（0-6，默认：1）</n>

**SSL 选项：**

  -rpcssl                                  使用 OpenSSL（https）JSON-RPC 连接

  -rpcsslcertificatechainfile=<文件.cert>  服务器证书文件（默认：server.cert）

  -rpcsslprivatekeyfile=<文件.pem>         服务器私匙文件（默认：server.pem）

  -rpcsslciphers=<密码>                    可接受的密码（默认：TLSv1+HIGH:!SSLv2:!aNULL:!eNULL:!AH:!3DES:@STRENGTH）
```

## bitcoin-cli

Bitcoin Core包含的一个功能完备的RPC客户端,可以查询区块，钱包，交易等各项信息。比特币使用的是Json-RPC接口。

##### 1.命令示例
```
//获取height为0的区块哈希
./src/bitcoin-cli getblockhash 0

000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f

//获取已知某哈希的区块具体信息
./src/bitcoin-cli getblock 000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f

{
  "hash": "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f",
  "confirmations": 187783,
  "strippedsize": 285,
  "size": 285,
  "weight": 1140,
  "height": 0,
  "version": 1,
  "versionHex": "00000001",
  "merkleroot": "4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b",
  "tx": [
    "4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"
  ],
  "time": 1231006505,
  "mediantime": 1231006505,
  "nonce": 2083236893,
  "bits": "1d00ffff",
  "difficulty": 1,
  "chainwork": "0000000000000000000000000000000000000000000000000000000100010001",
  "nextblockhash": "00000000839a8e6886ab5951d76f411475428afc90947ee320161bbf18eb6048"
}

```

##### 2.其他命令
```
A、一般性的命令
//帮助指令
help ( "command" )

stop         //停止bitcoin服务
getinfo      //获取当前节点信息
ping
getnettotals
getnetworkinfo
getpeerinfo
getconnectioncount
verifychain ( checklevel numblocks )
getaddednodeinfo dns ( "node" )
addnode "node" "add|remove|onetry"


B、钱包、账户、地址、转帐、发消息

getwalletinfo       //获取钱包信息
walletpassphrase "passphrase" timeout     //解锁钱包  密码-多久钱包自动锁定
walletlock          //锁定钱包
walletpassphrasechange "oldpassphrase" "newpassphrase"     //更改钱包密码
backupwallet "destination"      //钱包备份
importwallet "filename"            //钱包备份文件导入
dumpwallet "filename"             //钱包恢复

listaccounts ( minconf )        //列出所有用户
getaddressesbyaccount "account"   //列出用户所有的钱包地址
getaccountaddress "account"
getaccount "bitcoinaddress"
validateaddress "bitcoinaddress"
dumpprivkey "bitcoinaddress"
setaccount "bitcoinaddress" "account"
getnewaddress ( "account" )      //获取新的钱包地址，由钱包地址池生成
keypoolrefill ( newsize )
importprivkey "bitcoinprivkey" ( "label" rescan )      //导入私钥
createmultisig nrequired ["key",...]
addmultisigaddress nrequired ["key",...] ( "account" )

getbalance ( "account" minconf )    //显示所有经过至少minconf个确认的交易加和后的余额
getunconfirmedbalance

//获取account或bitcoinaddress对应收到的比特币数量
getreceivedbyaccount "account" ( minconf )
getreceivedbyaddress "bitcoinaddress" ( minconf )

listreceivedbyaccount ( minconf includeempty )
listreceivedbyaddress ( minconf includeempty )
move "fromaccount" "toaccount" amount ( minconf "comment" )
listunspent ( minconf maxconf  ["address",...] )
listlockunspent
lockunspent unlock [{"txid":"txid","vout":n},...]

getrawchangeaddress
listaddressgroupings
settxfee amount
sendtoaddress "bitcoinaddress" amount ( "comment" "comment-to" )
sendfrom "fromaccount" "tobitcoinaddress" amount ( minconf "comment" "comment-to" )
sendmany "fromaccount" {"address":amount,...} ( minconf "comment" )

signmessage "bitcoinaddress" "message"
verifymessage "bitcoinaddress" "signature" "message"



C、Tx、Block、Ming

createrawtransaction [{"txid":"id","vout":n},...] {"address":amount,...}
signrawtransaction "hexstring" ( [{"txid":"id","vout":n,"scriptPubKey":"hex","redeemScript":"hex"},...] ["privatekey1",...] sighashtype )
sendrawtransaction "hexstring" ( allowhighfees )


gettransaction "txid"   //获取简化版交易信息
//获取完整交易信息
getrawtransaction "txid" ( verbose )  //得到一个描述交易的16进制字符串
decoderawtransaction "hexstring"    //解码上面的字符串
listtransactions ( "account" count from )   //获取钱包对应的交易
listsinceblock ( "blockhash" target-confirmations )


getrawmempool ( verbose )
gettxoutsetinfo
gettxout "txid" n ( includemempool )

decodescript "hex"

getblockchaininfo
getblockcount
getbestblockhash
getblockhash index
getblock "hash" ( verbose )

getmininginfo
getdifficulty
getnetworkhashps ( blocks height )
gethashespersec                                                                                                                                                       
getgenerate
setgenerate generate ( genproclimit )
getwork ( "data" )
getblocktemplate ( "jsonrequestobject" )
submitblock "hexdata" ( "jsonparametersobject" )
```

Mac下bitcoin源码的编译就完成了，同时我们也了解了bitcoind和bitcoin-cli基本命令。万里长征终于迈出了第一步，今天就到这里了。


