# 0x03Transaction

首先，通过[blockchain.info](https://blockchain.info/)查看一笔交易的基本数据结构：

![blockchain.info_1](https://upload-images.jianshu.io/upload_images/830585-c19a462af97cef40.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

![blockchain.info_2](https://upload-images.jianshu.io/upload_images/830585-465046f81aeec2fb.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

![blockchain.info_3](https://upload-images.jianshu.io/upload_images/830585-a3a9db8ad53d180f.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

# 源码初窥

- ___代码路径: bitcoin/src/private___

### COutPut
```C++
/** An outpoint - a combination of a transaction hash and an index n into its vout 
*
** 一个交易哈希值与输出下标的集合
*/
class COutPoint
{
public:
    uint256 hash;       //交易哈西
    uint32_t n;         //对应序列号

    COutPoint(): n((uint32_t) -1) { }       
    COutPoint(const uint256& hashIn, uint32_t nIn): hash(hashIn), n(nIn) { }

    ADD_SERIALIZE_METHODS;      //用来序列化数据结构，方便存储和传输

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(hash);
        READWRITE(n);
    }

    void SetNull() { hash.SetNull(); n = (uint32_t) -1; }
    bool IsNull() const { return (hash.IsNull() && n == (uint32_t) -1); }

    //小于号<重载函数
    friend bool operator<(const COutPoint& a, const COutPoint& b)
    {
        int cmp = a.hash.Compare(b.hash);
        return cmp < 0 || (cmp == 0 && a.n < b.n);
    }

    //==重载函数
    friend bool operator==(const COutPoint& a, const COutPoint& b)
    {
        return (a.hash == b.hash && a.n == b.n);
    }

    //!=重载函数
    friend bool operator!=(const COutPoint& a, const COutPoint& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
};
```

### CTxIn
```C++
/** An input of a transaction.  It contains the location of the previous
 * transaction's output that it claims and a signature that matches the
 * output's public key.
 *
 **交易的输入，包括当前输入所对应上一笔交易的输出位置，
 *并且还包括上一笔输出所需要的签名脚本
 */
class CTxIn
{
public:
    COutPoint prevout;      //上一笔交易输出位置
    CScript scriptSig;      //解锁脚本
    uint32_t nSequence;     /**序列号，可用于交易的锁定 
                            nSequence字段的设计初心是想让交易能在在内存中修改，可惜后面从未运用过
                            对于具有nLocktime或CHECKLOCKTIMEVERIFY的交易，
                            nSequence值必须设置为小于2^32，以使时间锁定器有效。通常设置为2^32-1
                            由于BIP-68的激活，新的共识规则适用于任何包含nSequence值小于2^31的输入的交易（bit 1<<31 is not set）。
                            以编程方式，这意味着如果没有设置最高有效（bit 1<<31），它是一个表示“相对锁定时间”的标志。
                            否则（bit 1<<31set），nSequence值被保留用于其他用途，
                            例如启用CHECKLOCKTIMEVERIFY，nLocktime，Opt-In-Replace-By-Fee以及其他未来的新产品。
                            一笔输入交易，当输入脚本中的nSequence值小于2^31时，就是相对时间锁定的输入交易。
                            这种交易只有到了相对锁定时间后才生效。例如，
                            具有30个区块的nSequence相对时间锁的一个输入的交易
                            只有在从输入中引用的UTXO开始的时间起至少有30个块时才有效。
                            由于nSequence是每个输入字段，因此交易可能包含任何数量的时间锁定输入，
                            所有这些都必须具有足够的时间以使交易有效。
                            */
    CScriptWitness scriptWitness; //! Only serialized through CTransaction

    /* Setting nSequence to this value for every input in a transaction
     * disables nLockTime. 
     *
     * 规则1:如果一笔交易中所有的SEQUENCE_FINAL都被赋值了相应的nSequence，那么nLockTime就会被禁用
     */
    static const uint32_t SEQUENCE_FINAL = 0xffffffff;

    /* Below flags apply in the context of BIP 68*/
    /* If this flag set, CTxIn::nSequence is NOT interpreted as a
     * relative lock-time. 
     *
     * 规则2:如果设置了该值，nSequence不被用于相对时间锁定。规则1失效
     */
    static const uint32_t SEQUENCE_LOCKTIME_DISABLE_FLAG = (1 << 31);

    /* If CTxIn::nSequence encodes a relative lock-time and this flag
     * is set, the relative lock-time has units of 512 seconds,
     * otherwise it specifies blocks with a granularity of 1. 
     *
     * 规则3：如果规则1有效并且设置了此变量，那么相对锁定时间单位为512秒，否则锁定时间就为1个区块
     */
    static const uint32_t SEQUENCE_LOCKTIME_TYPE_FLAG = (1 << 22);

    /* If CTxIn::nSequence encodes a relative lock-time, this mask is
     * applied to extract that lock-time from the sequence field. 
     *
     * 规则4：如果nSequence用于相对时间锁，即规则1有效，那么这个变量就用来从nSequence计算对应的锁定时间
     */
    static const uint32_t SEQUENCE_LOCKTIME_MASK = 0x0000ffff;

    /* In order to use the same number of bits to encode roughly the
     * same wall-clock duration, and because blocks are naturally
     * limited to occur every 600s on average, the minimum granularity
     * for time-based relative lock-time is fixed at 512 seconds.
     * Converting from CTxIn::nSequence to seconds is performed by
     * multiplying by 512 = 2^9, or equivalently shifting up by
     * 9 bits. 
     *
     * 相对时间锁粒度
     * 为了使用相同的位数来粗略地编码相同的挂钟时间，
     * 因为区块的产生限制于每600s产生一个，
     * 相对时间锁定的最小单位为512是，512 = 2^9
     * 所以相对时间锁定的时间转化为相当于当前值左移9位
     */
    static const int SEQUENCE_LOCKTIME_GRANULARITY = 9;

    CTxIn()
    {
        nSequence = SEQUENCE_FINAL;
    }

    explicit CTxIn(COutPoint prevoutIn, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=SEQUENCE_FINAL);
    CTxIn(uint256 hashPrevTx, uint32_t nOut, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=SEQUENCE_FINAL);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(prevout);
        READWRITE(scriptSig);
        READWRITE(nSequence);
    }

    friend bool operator==(const CTxIn& a, const CTxIn& b)
    {
        return (a.prevout   == b.prevout &&
                a.scriptSig == b.scriptSig &&
                a.nSequence == b.nSequence);
    }

    friend bool operator!=(const CTxIn& a, const CTxIn& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
};
```

### CTxOut
```C++
/** An output of a transaction.  It contains the public key that the next input
 * must be able to sign with to claim it.
 *
 **交易输出，包含输出金额和锁定脚本
 */
class CTxOut
{
public:
    CAmount nValue;             //输出金额
    CScript scriptPubKey;       //锁定脚本

    CTxOut()
    {
        SetNull();
    }

    CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(nValue);
        READWRITE(scriptPubKey);
    }

    void SetNull()
    {
        nValue = -1;
        scriptPubKey.clear();
    }

    bool IsNull() const
    {
        return (nValue == -1);
    }

    friend bool operator==(const CTxOut& a, const CTxOut& b)
    {
        return (a.nValue       == b.nValue &&
                a.scriptPubKey == b.scriptPubKey);
    }

    friend bool operator!=(const CTxOut& a, const CTxOut& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
};
```

### CTransaction
```C++
/** The basic transaction that is broadcasted on the network and contained in
 * blocks.  A transaction can contain multiple inputs and outputs.
 *
 *
 ** 基本的交易，就是那些在网络中广播并被最终打包到区块中的数据结构。
 *  一个交易可以包含多个交易输入和输出
 */
class CTransaction
{
public:
    // Default transaction version.
    static const int32_t CURRENT_VERSION=2;         //默认交易版本

    // Changing the default transaction version requires a two step process: first
    // adapting relay policy by bumping MAX_STANDARD_VERSION, and then later date
    // bumping the default CURRENT_VERSION at which point both CURRENT_VERSION and
    // MAX_STANDARD_VERSION will be equal.
    /** 更改默认交易版本需要两个步骤：
    *   1.首先通过碰撞MAX_STANDARD_VERSION来调整中继策略，
    *   2.然后在稍后的日期碰撞默认的CURRENT_VERSION
    *   
    *   最终MAX_STANDARD_VERSION和CURRENT_VERSION会一致
    */
    static const int32_t MAX_STANDARD_VERSION=2;    

    // The local variables are made const to prevent unintended modification
    // without updating the cached hash value. However, CTransaction is not
    // actually immutable; deserialization and assignment are implemented,
    // and bypass the constness. This is safe, as they update the entire
    // strcture, including the hash.
    /** 下面这些变量都被定义为常量类型，从而避免无意识的修改了交易而没有更新缓存的hash值；
    *   然而CTransaction不是可变的
    *   反序列化和分配被执行的时候会绕过常量
    *   这才是安全的，因为更新整个结构包括哈希值
    */
    const std::vector<CTxIn> vin;       //交易输入
    const std::vector<CTxOut> vout;     //交易输出
    const int32_t nVersion;             //版本         
    const uint32_t nLockTime;           //锁定时间

private:
    /** Memory only. */
    const uint256 hash;

    uint256 ComputeHash() const;

public:
    /** Construct a CTransaction that qualifies as IsNull() */
    CTransaction();

    /** Convert a CMutableTransaction into a CTransaction. */
    /**可变交易转换为交易*/
    CTransaction(const CMutableTransaction &tx);
    CTransaction(CMutableTransaction &&tx);

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        SerializeTransaction(*this, s);
    }

    /** This deserializing constructor is provided instead of an Unserialize method.
     *  Unserialize is not possible, since it would require overwriting const fields. 
     *
     ** 提供此反序列化构造函数而不是Unserialize方法。
     *  反序列化是不可能的，因为它需要覆盖const字段
     */
    template <typename Stream>
    CTransaction(deserialize_type, Stream& s) : CTransaction(CMutableTransaction(deserialize, s)) {}

    bool IsNull() const {
        return vin.empty() && vout.empty();
    }

    const uint256& GetHash() const {
        return hash;
    }

    // Compute a hash that includes both transaction and witness data
    uint256 GetWitnessHash() const;         //计算包含交易和witness数据的散列           

    // Return sum of txouts.
    CAmount GetValueOut() const;            //返回交易出书金额总和      
    // GetValueIn() is a method on CCoinsViewCache, because
    // inputs must be known to compute value in.

    /**
     * Get the total transaction size in bytes, including witness data.
     * "Total Size" defined in BIP141 and BIP144.
     * @return Total transaction size in bytes
     */
    unsigned int GetTotalSize() const;      // 返回交易大小

    bool IsCoinBase() const                 //判断是否是创币交易
    {
        return (vin.size() == 1 && vin[0].prevout.IsNull());
    }

    friend bool operator==(const CTransaction& a, const CTransaction& b)
    {
        return a.hash == b.hash;
    }

    friend bool operator!=(const CTransaction& a, const CTransaction& b)
    {
        return a.hash != b.hash;
    }

    std::string ToString() const;

    bool HasWitness() const
    {
        for (size_t i = 0; i < vin.size(); i++) {
            if (!vin[i].scriptWitness.IsNull()) {
                return true;
            }
        }
        return false;
    }
};
```

### CMutableTransaction

可变交易类，内容和CTransaction差不多。只是交易可以直接修改，广播中传播和打包到区块的交易都是CTransaction类型。

# 交易结构

交易是比特币的核心数据结构，包括区块在内的数据结构都是在为交易服务。

### 整体结构

数据项 | 大小(Byte) | 数据类型 | 描述
:-: | :-: | :-: | :-: 
Version | 4 | uint32_t  | 交易版本
tx_in count | Varies | [CompactSize Unsigned Integer](https://bitcoin.org/en/developer-reference#compactsize-unsigned-integers) | 交易输入量
tx_out count | Varies | [CompactSize Unsigned Integer](https://bitcoin.org/en/developer-reference#compactsize-unsigned-integers) | 交易输出量
tx_in | Varies | CTxIn | 交易输入
tx_in | Varies | CTxOut | 交易输出
lock_time | 4 | uint32_t | 交易锁定时间，详见[锁定规则](https://bitcoin.org/en/developer-guide#locktime-and-sequence-number)

#交易输入TxIn

数据项 | 大小(Byte) | 数据类型 | 描述
:-: | :-: | :-: | :-: 
previous_output | 36 | COutPoint | 上一个交易的输出
script bytes | Varies  < 10000 | CompactSize Unsigned Integer | 解锁脚本大小
[signature script](https://bitcoin.org/en/glossary/signature-script "Data generated by a spender which is almost always used as variables to satisfy a pubkey script. Signature Scripts are called scriptSig in code.") | Varies | char[] | 解锁脚本
sequence | 4| uint32_t | 序列号，可用于相对时间锁定

### 交易输出TxOut

数据项 | 大小(Byte) | 数据类型 | 描述
:-: | :-: | :-: | :-: 
value | 8 | int64_t | 交易输出，单位为Satoshis
pk_script bytes | Varies  < 10000 | CompactSize Unsigned Integer | 锁定脚本大小
pk_script | Varies | char[] | 锁定脚本，定义花费须满足的条件

# 创币交易CoinbaseTransaction

每一个区块内包含的第一条交易为CoinbaseTransaction，它作为对挖出该区块的矿工的比特币奖励交易。
- 没有TxIn
- 交易哈希为0
- 输出的索引值固定，为0xffffffff
- 大端存储
- BIP34规定增加一个4字节的height的字段，现在这个参数是必须的







