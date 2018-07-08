# 0x04TransactionPool

了解了区块和交易的数据结构，接下来就是介于这两者之间的一个重要的数据结构：交易池。

当比特币网络把某个时刻产生的交易广播到网络时，矿工接收到交易后并不是立即打包到备选区块。而是将接收到的交易放到类似缓冲区的一个交易池里，然后会根据一定的优先顺序来选择交易打包，以此来保障自己能获得尽可能多的交易费。

所以了解交易池的数据结构，对理解矿工打包交易会有很大的裨益。

# 找啊找啊找交易池
首先我猜测有关交易池的类可能叫transactionPool，于是我试着全局搜这个词：

![悲剧了，宝宝没找到啊](https://upload-images.jianshu.io/upload_images/830585-fe7f75687cac62c1.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

于是，我扩大搜索范围，搜索pool。搜到的结果很多，我大概地往下翻，试图寻找是交易池的那个。找到很多处mempool，于是我点进去试着搜索mempool，找到了txmempool.h这个头文件，交易内存池，应该就是这个了。

![image.png](https://upload-images.jianshu.io/upload_images/830585-fbea24ac7fdd0f7c.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)


# 源码初窥
- ___代码路径: bitcoin/src/txmempool.h___

### LockPoints
```C++
/** Fake height value used in Coin to signify they are only in the memory pool (since 0.8) */
//一个"假"的高度值，用来标识它们只存在于交易池中
static const uint32_t MEMPOOL_HEIGHT = 0x7FFFFFFF;

//交易锁定点,交易最后的区块高度和打包时间
struct LockPoints
{
    // Will be set to the blockchain height and median time past
    // values that would be necessary to satisfy all relative locktime
    // constraints (BIP68) of this tx given our view of block chain history
    /**
    *   将设置为区块链高度和中值时间过去值,
    *   这些值对于满足tx相对时间锁是至关重要的(BIP68)
    *   
    */
    int height;
    int64_t time;
    // As long as the current chain descends from the highest height block
    // containing one of the inputs used in the calculation, then the cached
    // values are still valid even after a reorg.
    /**
    *   只要当前链包含计算中使用的某个输入的最高快高度，
    *   则即使在链重新构建后缓存的值依然有效
    */
    CBlockIndex* maxInputBlock;

    LockPoints() : height(0), time(0), maxInputBlock(nullptr) { }
};
```

### CTxMemPoolEntry

```C++
class CTxMemPool;

/** \class CTxMemPoolEntry
 *
 * CTxMemPoolEntry stores data about the corresponding transaction, as well
 * as data about all in-mempool transactions that depend on the transaction
 * ("descendant" transactions).
 *
 * When a new entry is added to the mempool, we update the descendant state
 * (nCountWithDescendants, nSizeWithDescendants, and nModFeesWithDescendants) for
 * all ancestors of the newly added transaction.
 *
 **CTxMemPoolEntry 存储相应的交易
 * 以及该交易对应的所有子孙交易
 *
 * 当一个新的CTxMemPoolEntry被添加到交易池，我们会更新新添加交易的所有子孙交易的状态
 * (包括子孙交易数量，大小，和交易费用）和祖父交易状态
 */

//交易池基本构成元素
class CTxMemPoolEntry
{
private:
    CTransactionRef tx;         //交易引用
    CAmount nFee;               //交易费用      //!< Cached to avoid expensive parent-transaction lookups
    size_t nTxWeight;           //            //!< ... and avoid recomputing tx weight (also used for GetTxSize())
    size_t nUsageSize;          //大小        //!< ... and total memory usage
    int64_t nTime;              //交易时间戳   //!< Local time when entering the mempool
    unsigned int entryHeight;   //区块高度  //!< Chain height when entering the mempool
    bool spendsCoinbase;        //上个交易是否是创币交易   //!< keep track of transactions that spend a coinbase
    int64_t sigOpCost;          //？？？  !< Total sigop cost
    int64_t feeDelta;           //交易优先级的一个标量    //!< Used for determining the priority of the transaction for mining in a block
    LockPoints lockPoints;      //锁定点，交易最后的区块高度和打包时间 //!< Track the height and time at which tx was final

    // Information about descendants of this transaction that are in the
    // mempool; if we remove this transaction we must remove all of these
    // descendants as well.
    /* 
    **  子孙交易信息
    *   如果我们移除一个交易，我们也必须同时移除它所有的子孙交易
    */
    uint64_t nCountWithDescendants;     //子孙交易数量 //!< number of descendant transactions
    uint64_t nSizeWithDescendants;      //大小        //!< ... and size
    CAmount nModFeesWithDescendants;    //费用总和，包括当前交易   //!< ... and total fees (all including us)

    // Analogous statistics for ancestor transactions
    //祖先交易信息
    uint64_t nCountWithAncestors;       //祖先交易数量
    uint64_t nSizeWithAncestors;        //大小
    CAmount nModFeesWithAncestors;      //费用总和
    int64_t nSigOpCostWithAncestors;    //？？？ 

public:
    CTxMemPoolEntry(const CTransactionRef& _tx, const CAmount& _nFee,
                    int64_t _nTime, unsigned int _entryHeight,
                    bool spendsCoinbase,
                    int64_t nSigOpsCost, LockPoints lp);

    const CTransaction& GetTx() const { return *this->tx; }
    CTransactionRef GetSharedTx() const { return this->tx; }
    const CAmount& GetFee() const { return nFee; }
    size_t GetTxSize() const;
    size_t GetTxWeight() const { return nTxWeight; }
    int64_t GetTime() const { return nTime; }
    unsigned int GetHeight() const { return entryHeight; }
    int64_t GetSigOpCost() const { return sigOpCost; }
    int64_t GetModifiedFee() const { return nFee + feeDelta; }
    size_t DynamicMemoryUsage() const { return nUsageSize; }
    const LockPoints& GetLockPoints() const { return lockPoints; }

    // Adjusts the descendant state.
    // 更新子孙交易状态
    void UpdateDescendantState(int64_t modifySize, CAmount modifyFee, int64_t modifyCount);
    // Adjusts the ancestor state
    // 更新祖先交易状态
    void UpdateAncestorState(int64_t modifySize, CAmount modifyFee, int64_t modifyCount, int64_t modifySigOps);
    // Updates the fee delta used for mining priority score, and the
    // modified fees with descendants.
    // 更新交易优先级
    void UpdateFeeDelta(int64_t feeDelta);
    // Update the LockPoints after a reorg
    // 更新锁定点
    void UpdateLockPoints(const LockPoints& lp);

    //获取子孙交易信息
    uint64_t GetCountWithDescendants() const { return nCountWithDescendants; }
    uint64_t GetSizeWithDescendants() const { return nSizeWithDescendants; }
    CAmount GetModFeesWithDescendants() const { return nModFeesWithDescendants; }

    bool GetSpendsCoinbase() const { return spendsCoinbase; }

    //获取祖先交易信息
    uint64_t GetCountWithAncestors() const { return nCountWithAncestors; }
    uint64_t GetSizeWithAncestors() const { return nSizeWithAncestors; }
    CAmount GetModFeesWithAncestors() const { return nModFeesWithAncestors; }
    int64_t GetSigOpCostWithAncestors() const { return nSigOpCostWithAncestors; }

    mutable size_t vTxHashesIdx;    //交易池哈希的下标  //!< Index in mempool's vTxHashes
};
```

### CTxMemPoolEntry几种排序方法

```C++

/** \class CompareTxMemPoolEntryByDescendantScore
 *
 *  Sort an entry by max(score/size of entry's tx, score/size with all descendants).
 *
 ** 按score/size原则对CTxMemPoolEntry排序
 */
class CompareTxMemPoolEntryByDescendantScore
{
public:
    bool operator()(const CTxMemPoolEntry& a, const CTxMemPoolEntry& b) const
    {
        double a_mod_fee, a_size, b_mod_fee, b_size;

        GetModFeeAndSize(a, a_mod_fee, a_size);
        GetModFeeAndSize(b, b_mod_fee, b_size);

        // Avoid division by rewriting (a/b > c/d) as (a*d > c*b).
        double f1 = a_mod_fee * b_size;
        double f2 = a_size * b_mod_fee;

        if (f1 == f2) {
            return a.GetTime() >= b.GetTime();
        }
        return f1 < f2;
    }

    // Return the fee/size we're using for sorting this entry.
    void GetModFeeAndSize(const CTxMemPoolEntry &a, double &mod_fee, double &size) const
    {
        // Compare feerate with descendants to feerate of the transaction, and
        // return the fee/size for the max.
        double f1 = (double)a.GetModifiedFee() * a.GetSizeWithDescendants();
        double f2 = (double)a.GetModFeesWithDescendants() * a.GetTxSize();

        if (f2 > f1) {
            mod_fee = a.GetModFeesWithDescendants();
            size = a.GetSizeWithDescendants();
        } else {
            mod_fee = a.GetModifiedFee();
            size = a.GetTxSize();
        }
    }
};

/** \class CompareTxMemPoolEntryByScore
 *
 *  Sort by score of entry ((fee+delta)/size) in descending order
 **
 *  按(fee+delta)/size原则对CTxMemPoolEntry排序
 */
class CompareTxMemPoolEntryByScore
{
public:
    bool operator()(const CTxMemPoolEntry& a, const CTxMemPoolEntry& b) const
    {
        double f1 = (double)a.GetModifiedFee() * b.GetTxSize();
        double f2 = (double)b.GetModifiedFee() * a.GetTxSize();
        if (f1 == f2) {
            return b.GetTx().GetHash() < a.GetTx().GetHash();
        }
        return f1 > f2;
    }
};

//按时间CTxMemPoolEntry对排序
class CompareTxMemPoolEntryByEntryTime
{
public:
    bool operator()(const CTxMemPoolEntry& a, const CTxMemPoolEntry& b) const
    {
        return a.GetTime() < b.GetTime();
    }
};

/** \class CompareTxMemPoolEntryByAncestorScore
 *
 *  Sort an entry by min(score/size of entry's tx, score/size with all ancestors).
 */
class CompareTxMemPoolEntryByAncestorFee
{
public:
    template<typename T>
    bool operator()(const T& a, const T& b) const
    {
        double a_mod_fee, a_size, b_mod_fee, b_size;

        GetModFeeAndSize(a, a_mod_fee, a_size);
        GetModFeeAndSize(b, b_mod_fee, b_size);

        // Avoid division by rewriting (a/b > c/d) as (a*d > c*b).
        double f1 = a_mod_fee * b_size;
        double f2 = a_size * b_mod_fee;

        if (f1 == f2) {
            return a.GetTx().GetHash() < b.GetTx().GetHash();
        }
        return f1 > f2;
    }

    // Return the fee/size we're using for sorting this entry.
    template <typename T>
    void GetModFeeAndSize(const T &a, double &mod_fee, double &size) const
    {
        // Compare feerate with ancestors to feerate of the transaction, and
        // return the fee/size for the min.
        double f1 = (double)a.GetModifiedFee() * a.GetSizeWithAncestors();
        double f2 = (double)a.GetModFeesWithAncestors() * a.GetTxSize();

        if (f1 > f2) {
            mod_fee = a.GetModFeesWithAncestors();
            size = a.GetSizeWithAncestors();
        } else {
            mod_fee = a.GetModifiedFee();
            size = a.GetTxSize();
        }
    }
};
```

### TxMempoolInfo

```C++
/**
 * Information about a mempool transaction.
 * 交易进入内存池的信息
 */
struct TxMempoolInfo
{
    /** The transaction itself */
    CTransactionRef tx;     //交易引用

    /** Time the transaction entered the mempool. */
    int64_t nTime;          //交易进入内存池时间

    /** Feerate of the transaction. */
    CFeeRate feeRate;       //交易费率

    /** The fee delta. */   
    int64_t nFeeDelta;      //交易优先级
};
```

### MemPoolRemovalReason
```C++
/** Reason why a transaction was removed from the mempool,
 * this is passed to the notification signal.
 *
 * 交易被移出内存池的原因
 */
enum class MemPoolRemovalReason {
    UNKNOWN = 0,        //未知原因     //! Manually removed or unknown reason
    EXPIRY,             //过期        //! Expired from mempool
    SIZELIMIT,          //大小限制     //! Removed in size limiting
    REORG,              //被重组      //! Removed for reorganization
    BLOCK,              //因为区块    //! Removed for block
    CONFLICT,           //区块内交易冲突//! Removed for conflict with in-block transaction
    REPLACED            //被替代       //! Removed for replacement
};
```

### CTxMemPool
```C++
/**
 * CTxMemPool stores valid-according-to-the-current-best-chain transactions
 * that may be included in the next block.
 *
 * CTxMemPool 保存当前主链所有的交易。这些交易有可能被加入到下一个有效区块中
 *
 * Transactions are added when they are seen on the network (or created by the
 * local node), but not all transactions seen are added to the pool. For
 * example, the following new transactions will not be added to the mempool:
 * - a transaction which doesn't meet the minimum fee requirements.
 * - a new transaction that double-spends an input of a transaction already in
 * the pool where the new transaction does not meet the Replace-By-Fee
 * requirements as defined in BIP 125.
 * - a non-standard transaction.
 *
 **当交易在比特币网络上广播时会被加入到交易池。
 * 比如以下新的交易将不会被加入到交易池中：
 *  - 1.没有满足最低交易费的交易
 *  - 2."双花"交易
 *  - 3.一个非标准交易
 *
 * CTxMemPool::mapTx, and CTxMemPoolEntry bookkeeping:
 *
 * mapTx is a boost::multi_index that sorts the mempool on 4 criteria:
 * - transaction hash       //交易hash
 * - //交易费率(包括所有子孙交易)
 * - feerate [we use max(feerate of tx, feerate of tx with all descendants)]
 * - time in mempool        //加入交易池的时间
 *
 * Note: the term "descendant" refers to in-mempool transactions that depend on
 * this one, while "ancestor" refers to in-mempool transactions that a given
 * transaction depends on.
 *
 * In order for the feerate sort to remain correct, we must update transactions
 * in the mempool when new descendants arrive.  To facilitate this, we track
 * the set of in-mempool direct parents and direct children in mapLinks.  Within
 * each CTxMemPoolEntry, we track the size and fees of all descendants.
 *
 ** 为了保障交易费的正确性，当新交易被加入到交易池时，我们必须更新该交易的所有祖先交易和子孙交易。
 *  
 * Usually when a new transaction is added to the mempool, it has no in-mempool
 * children (because any such children would be an orphan).  So in
 * addUnchecked(), we:
 * - update a new entry's setMemPoolParents to include all in-mempool parents
 * - update the new entry's direct parents to include the new tx as a child
 * - update all ancestors of the transaction to include the new tx's size/fee
 *
 * When a transaction is removed from the mempool, we must:
 * - update all in-mempool parents to not track the tx in setMemPoolChildren
 * - update all ancestors to not include the tx's size/fees in descendant state
 * - update all in-mempool children to not include it as a parent
 *
 * These happen in UpdateForRemoveFromMempool().  (Note that when removing a
 * transaction along with its descendants, we must calculate that set of
 * transactions to be removed before doing the removal, or else the mempool can
 * be in an inconsistent state where it's impossible to walk the ancestors of
 * a transaction.)
 *
 * In the event of a reorg, the assumption that a newly added tx has no
 * in-mempool children is false.  In particular, the mempool is in an
 * inconsistent state while new transactions are being added, because there may
 * be descendant transactions of a tx coming from a disconnected block that are
 * unreachable from just looking at transactions in the mempool (the linking
 * transactions may also be in the disconnected block, waiting to be added).
 * Because of this, there's not much benefit in trying to search for in-mempool
 * children in addUnchecked().  Instead, in the special case of transactions
 * being added from a disconnected block, we require the caller to clean up the
 * state, to account for in-mempool, out-of-block descendants for all the
 * in-block transactions by calling UpdateTransactionsFromBlock().  Note that
 * until this is called, the mempool state is not consistent, and in particular
 * mapLinks may not be correct (and therefore functions like
 * CalculateMemPoolAncestors() and CalculateDescendants() that rely
 * on them to walk the mempool are not generally safe to use).
 *
 * Computational limits:
 *
 * Updating all in-mempool ancestors of a newly added transaction can be slow,
 * if no bound exists on how many in-mempool ancestors there may be.
 * CalculateMemPoolAncestors() takes configurable limits that are designed to
 * prevent these calculations from being too CPU intensive.
 *
 */
class CTxMemPool
{
private:
    uint32_t nCheckFrequency;           //2^32时间检查的次数   //!< Value n means that n times in 2^32 we check.
    unsigned int nTransactionsUpdated;  //!< Used by getblocktemplate to trigger CreateNewBlock() invocation
    CBlockPolicyEstimator* minerPolicyEstimator;

    uint64_t totalTxSize;      //交易池虚拟大小，不包括见证数据 //!< sum of all mempool tx's virtual sizes. Differs from serialized tx size since witness data is discounted. Defined in BIP 141.
    uint64_t cachedInnerUsage; //map使用的动态内存大小        //!< sum of dynamic memory usage of all the map elements (NOT the maps themselves)

    mutable int64_t lastRollingFeeUpdate;
    mutable bool blockSinceLastRollingFeeBump;
    mutable double rollingMinimumFeeRate;   //进入交易池需要满足的最小费用    //!< minimum fee to get into the pool, decreases exponentially

    void trackPackageRemoved(const CFeeRate& rate);

public:

    static const int ROLLING_FEE_HALFLIFE = 60 * 60 * 12; // public only for testing

    typedef boost::multi_index_container<
        CTxMemPoolEntry,
        boost::multi_index::indexed_by<
            // sorted by txid   根据交易哈希排序
            boost::multi_index::hashed_unique<mempoolentry_txid, SaltedTxidHasher>,
            // sorted by fee rate   交易费
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<descendant_score>,
                boost::multi_index::identity<CTxMemPoolEntry>,
                CompareTxMemPoolEntryByDescendantScore
            >,
            // sorted by entry time 进入交易池的时间
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<entry_time>,
                boost::multi_index::identity<CTxMemPoolEntry>,
                CompareTxMemPoolEntryByEntryTime
            >,
            // sorted by fee rate with ancestors    祖父交易交易费
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ancestor_score>,
                boost::multi_index::identity<CTxMemPoolEntry>,
                CompareTxMemPoolEntryByAncestorFee
            >
        >
    > indexed_transaction_set;

    mutable CCriticalSection cs;
    indexed_transaction_set mapTx;

    typedef indexed_transaction_set::nth_index<0>::type::iterator txiter;
    std::vector<std::pair<uint256, txiter> > vTxHashes; //见证数据的哈希   //!< All tx witness hashes/entries in mapTx, in random order

    struct CompareIteratorByHash {
        bool operator()(const txiter &a, const txiter &b) const {
            return a->GetTx().GetHash() < b->GetTx().GetHash();
        }
    };
    typedef std::set<txiter, CompareIteratorByHash> setEntries;

    const setEntries & GetMemPoolParents(txiter entry) const;
    const setEntries & GetMemPoolChildren(txiter entry) const;
private:
    typedef std::map<txiter, setEntries, CompareIteratorByHash> cacheMap;

    struct TxLinks {
        setEntries parents;
        setEntries children;
    };

    typedef std::map<txiter, TxLinks, CompareIteratorByHash> txlinksMap;
    txlinksMap mapLinks;

    void UpdateParent(txiter entry, txiter parent, bool add);
    void UpdateChild(txiter entry, txiter child, bool add);

    std::vector<indexed_transaction_set::const_iterator> GetSortedDepthAndScore() const;

public:
    indirectmap<COutPoint, const CTransaction*> mapNextTx;
    std::map<uint256, CAmount> mapDeltas;

    /** Create a new CTxMemPool.
    *   创建一个新的交易池
     */
    explicit CTxMemPool(CBlockPolicyEstimator* estimator = nullptr);

    /**
     * If sanity-checking is turned on, check makes sure the pool is
     * consistent (does not contain two transactions that spend the same inputs,
     * all inputs are in the mapNextTx array). If sanity-checking is turned off,
     * check does nothing.
     *
     **如果开启了sanity-check，check函数将保证pool的一致性(不包含两个在同一个输入中的交易)
     * 所有的输入都在mapNextTx数组里；sanity-check关闭，check函数无效
     *
     */
    void check(const CCoinsViewCache *pcoins) const;
    void setSanityCheck(double dFrequency = 1.0) { nCheckFrequency = static_cast<uint32_t>(dFrequency * 4294967295.0); }

    // addUnchecked must updated state for all ancestors of a given transaction,
    // to track size/count of descendant transactions.  First version of
    // addUnchecked can be used to have it call CalculateMemPoolAncestors(), and
    // then invoke the second version.
    // Note that addUnchecked is ONLY called from ATMP outside of tests
    // and any other callers may break wallet's in-mempool tracking (due to
    // lack of CValidationInterface::TransactionAddedToMempool callbacks).
    /**
    *  addUnchecked函数必先更新祖先交易的状态
    *  第一个addUnchecked函数可以用来调用CalculateMemPoolAncestors
    *  然后再调用第二个addUnchecked
    */
    bool addUnchecked(const uint256& hash, const CTxMemPoolEntry &entry, bool validFeeEstimate = true);
    bool addUnchecked(const uint256& hash, const CTxMemPoolEntry &entry, setEntries &setAncestors, bool validFeeEstimate = true);

    void removeRecursive(const CTransaction &tx, MemPoolRemovalReason reason = MemPoolRemovalReason::UNKNOWN);
    void removeForReorg(const CCoinsViewCache *pcoins, unsigned int nMemPoolHeight, int flags);
    void removeConflicts(const CTransaction &tx);
    void removeForBlock(const std::vector<CTransactionRef>& vtx, unsigned int nBlockHeight);

    void clear();
    void _clear(); //lock free
    bool CompareDepthAndScore(const uint256& hasha, const uint256& hashb);
    void queryHashes(std::vector<uint256>& vtxid);
    bool isSpent(const COutPoint& outpoint);
    unsigned int GetTransactionsUpdated() const;
    void AddTransactionsUpdated(unsigned int n);
    /**
     * Check that none of this transactions inputs are in the mempool, and thus
     * the tx is not dependent on other mempool transactions to be included in a block.
     **
     * 检查交易的输入是否在当前交易池中
     */
    bool HasNoInputsOf(const CTransaction& tx) const;

    /** Affect CreateNewBlock prioritisation of transactions */
    //调整CreateNewBlock时的交易优先级
    void PrioritiseTransaction(const uint256& hash, const CAmount& nFeeDelta);
    void ApplyDelta(const uint256 hash, CAmount &nFeeDelta) const;
    void ClearPrioritisation(const uint256 hash);

public:
    /** Remove a set of transactions from the mempool.
     *  If a transaction is in this set, then all in-mempool descendants must
     *  also be in the set, unless this transaction is being removed for being
     *  in a block.
     *  Set updateDescendants to true when removing a tx that was in a block, so
     *  that any in-mempool descendants have their ancestor state updated.
     ** 
     *  从mempool中移除一个交易集合，
     *  如果一个交易在这个集合中，那么它的所有子孙交易都必须在集合中，
     *  除非该交易已经被打包到区块中。
     *  如果要移除一个已经被打包到区块中的交易，
     *  那么要把updateDescendants设为true，
     *  从而更新mempool中所有子孙节点的祖先信息
     */
    void RemoveStaged(setEntries &stage, bool updateDescendants, MemPoolRemovalReason reason = MemPoolRemovalReason::UNKNOWN);

    /** When adding transactions from a disconnected block back to the mempool,
     *  new mempool entries may have children in the mempool (which is generally
     *  not the case when otherwise adding transactions).
     *  UpdateTransactionsFromBlock() will find child transactions and update the
     *  descendant state for each transaction in vHashesToUpdate (excluding any
     *  child transactions present in vHashesToUpdate, which are already accounted
     *  for).  Note: vHashesToUpdate should be the set of transactions from the
     *  disconnected block that have been accepted back into the mempool.
     **
     *  更新每一个交易的所有子孙交易状态
     *
     */
    void UpdateTransactionsFromBlock(const std::vector<uint256> &vHashesToUpdate);

    /** Try to calculate all in-mempool ancestors of entry.
     *  (these are all calculated including the tx itself)
     *  limitAncestorCount = max number of ancestors
     *  limitAncestorSize = max size of ancestors
     *  limitDescendantCount = max number of descendants any ancestor can have
     *  limitDescendantSize = max size of descendants any ancestor can have
     *  errString = populated with error reason if any limits are hit
     *  fSearchForParents = whether to search a tx's vin for in-mempool parents, or
     *    look up parents from mapLinks. Must be true for entries not in the mempool
     ** 
     *  计算mempool中所有entry的祖先
     *  limitAncestorCount   = 最大祖先数量
     *  limitAncestorSize    = 最大祖先交易大小
     *  limitDescendantCount = 任意祖先的最大子孙数量
     *  limitDescendantSize  = 任意祖先的最大子孙大小
     *  errString            = 超过了任何limit限制的错误提示
     *  fSearchForParents    = 是否在mempool中搜索交易的输入，
     *  或者从mapLinks中查找，对于不在mempool中的entry必须设为true
     */
    bool CalculateMemPoolAncestors(const CTxMemPoolEntry &entry, setEntries &setAncestors, uint64_t limitAncestorCount, uint64_t limitAncestorSize, uint64_t limitDescendantCount, uint64_t limitDescendantSize, std::string &errString, bool fSearchForParents = true) const;

    /** Populate setDescendants with all in-mempool descendants of hash.
     *  Assumes that setDescendants includes all in-mempool descendants of anything
     *  already in it.  */
    //计算所有子孙交易
    void CalculateDescendants(txiter it, setEntries &setDescendants);

    /** The minimum fee to get into the mempool, which may itself not be enough
      *  for larger-sized transactions.
      *  The incrementalRelayFee policy variable is used to bound the time it
      *  takes the fee rate to go back down all the way to 0. When the feerate
      *  would otherwise be half of this, it is set to 0 instead.
      **
      * 获取进入交易池需要满足的最小交易费，本身可能不够适用于大型交易
      * incrementalRelayFee变量用来限制feerate降到0所需的时间
      * 当交易费是它的一半时，它被设置为0
      */
    CFeeRate GetMinFee(size_t sizelimit) const;

    /** Remove transactions from the mempool until its dynamic size is <= sizelimit.
      *  pvNoSpendsRemaining, if set, will be populated with the list of outpoints
      *  which are not in mempool which no longer have any spends in this mempool.
      ** 
      *  移除所有动态大小超过sizelimit的交易，
      *  如果传入了pvNoSpendsRemaining，那么将返回不在mempool中并且也没有
      *  任何输出在mempool的交易列表
      */
    void TrimToSize(size_t sizelimit, std::vector<COutPoint>* pvNoSpendsRemaining=nullptr);

    /** Expire all transaction (and their dependencies) in the mempool older than time. Return the number of removed transactions. */
    /*
    ** 移除所有在time之前的交易和它的子孙交易，
    *  并返回被移除交易的数量
    int Expire(int64_t time);

    /** Returns false if the transaction is in the mempool and not within the chain limit specified. */
    //如果交易不满足chain limit，返回false
    bool TransactionWithinChainLimit(const uint256& txid, size_t chainLimit) const;

    unsigned long size()
    {
        LOCK(cs);
        return mapTx.size();
    }

    uint64_t GetTotalTxSize() const
    {
        LOCK(cs);
        return totalTxSize;
    }

    bool exists(uint256 hash) const
    {
        LOCK(cs);
        return (mapTx.count(hash) != 0);
    }

    CTransactionRef get(const uint256& hash) const;
    TxMempoolInfo info(const uint256& hash) const;
    std::vector<TxMempoolInfo> infoAll() const;

    size_t DynamicMemoryUsage() const;

    boost::signals2::signal<void (CTransactionRef)> NotifyEntryAdded;
    boost::signals2::signal<void (CTransactionRef, MemPoolRemovalReason)> NotifyEntryRemoved;

private:
    /** UpdateForDescendants is used by UpdateTransactionsFromBlock to update
     *  the descendants for a single transaction that has been added to the
     *  mempool but may have child transactions in the mempool, eg during a
     *  chain reorg.  setExclude is the set of descendant transactions in the
     *  mempool that must not be accounted for (because any descendants in
     *  setExclude were added to the mempool after the transaction being
     *  updated and hence their state is already reflected in the parent
     *  state).
     *
     *  cachedDescendants will be updated with the descendants of the transaction
     *  being updated, so that future invocations don't need to walk the
     *  same transaction again, if encountered in another transaction chain.
     **
     *  UpdateForDescendants 是被 UpdateTransactionsFromBlock 调用，
     *  用来更新被加入pool中的单个交易的子孙节节点；
     *  setExclude 是内存池中不用更新的子孙交易集合 (because any descendants in
     *  setExclude were added to the mempool after the transaction being
     *  updated and hence their state is already reflected in the parent
     *  state).
     *
     *  当子孙交易被更新时，cachedDescendants也同时更新
     */
    void UpdateForDescendants(txiter updateIt,
            cacheMap &cachedDescendants,
            const std::set<uint256> &setExclude);
    /** Update ancestors of hash to add/remove it as a descendant transaction. */
    //更新一个祖先交易去添加或移除 为一个子孙交易
    void UpdateAncestorsOf(bool add, txiter hash, setEntries &setAncestors);
    /** Set ancestor state for an entry */
    //设置一个祖先交易
    void UpdateEntryForAncestors(txiter it, const setEntries &setAncestors);
    /** For each transaction being removed, update ancestors and any direct children.
      * If updateDescendants is true, then also update in-mempool descendants'
      * ancestor state. */
    /** 对于每一个要移除的交易，更新它的祖先和直接的儿子。
      * 如果updateDescendants 设为 true, 那么还同时更新mempool中子孙的祖先状态
    */
    void UpdateForRemoveFromMempool(const setEntries &entriesToRemove, bool updateDescendants);
    /** Sever link between specified transaction and direct children. */
    //切断指定交易与直接子女之间的链接
    void UpdateChildrenForRemoval(txiter entry);

    /** Before calling removeUnchecked for a given transaction,
     *  UpdateForRemoveFromMempool must be called on the entire (dependent) set
     *  of transactions being removed at the same time.  We use each
     *  CTxMemPoolEntry's setMemPoolParents in order to walk ancestors of a
     *  given transaction that is removed, so we can't remove intermediate
     *  transactions in a chain before we've updated all the state for the
     *  removal.
     ** 
     *  对于一个特定的交易，调用 removeUnchecked 之前，
     *  必须为同时为要移除的交易集合调用UpdateForRemoveFromMempool。
     *  我们使用每个CTxMemPoolEntry中的setMemPoolParents来遍历
     *  要移除交易的祖先，这样能保证我们更新的正确性。
     */
    void removeUnchecked(txiter entry, MemPoolRemovalReason reason = MemPoolRemovalReason::UNKNOWN);
};
```

有关交易池的概念，光头文件就900多行。我们先大概了解下一个交易池(TxMemPool)是由若干个CTxMemPoolEntry构成。然后对交易池某些关键函数知道其意思，以后具体遇到了再回过头来查看。

