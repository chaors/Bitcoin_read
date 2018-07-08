# 0x05Consensus

# 写在前面

最近有点懒散，竟然有一周没有读源码了。想来惭愧，今天重拾bitcoin源码，来看看比特币的共识机制。

我们都知道比特币采用的共识机制是工作量证明(POW),将区块链的记账的权利通过一个数学问题的解决来决定。前面，在用python搭建简单的区块链框架中，我们简单地通过一个简单的[🌰](https://www.jianshu.com/p/b39c361687c1)模拟了POW机制的挖矿原理。

# 源码初窥

### Consensus

- ___源码路径:.../src/consensus/params.h___

```C++
namespace Consensus {

enum DeploymentPos
{
    DEPLOYMENT_TESTDUMMY,
    DEPLOYMENT_CSV, // Deployment of BIP68, BIP112, and BIP113.
    DEPLOYMENT_SEGWIT, // Deployment of BIP141, BIP143, and BIP147.
    // NOTE: Also add new deployments to VersionBitsDeploymentInfo in versionbits.cpp
    MAX_VERSION_BITS_DEPLOYMENTS
};

/**
 * Struct for each individual consensus rule change using BIP9.
 *
 **使用bip9改变共识规则的结构体
 */
struct BIP9Deployment {
    /** Bit position to select the particular bit in nVersion. */
    /**用来标志nVersion中特定位的bit*/
    int bit;
    /** Start MedianTime for version bits miner confirmation. Can be a date in the past */
    /** 矿工确认版本位的平均开始时间。这个时间可以设置在过去*/
    int64_t nStartTime;
    /** Timeout/expiry MedianTime for the deployment attempt. */
    /** 尝试部署的平均超时时间*/
    int64_t nTimeout;

    /** Constant for nTimeout very far in the future. */
    static constexpr int64_t NO_TIMEOUT = std::numeric_limits<int64_t>::max();

    /** Special value for nStartTime indicating that the deployment is always active.
     *  This is useful for testing, as it means tests don't need to deal with the activation
     *  process (which takes at least 3 BIP9 intervals). Only tests that specifically test the
     *  behaviour during activation cannot use this. */
    static constexpr int64_t ALWAYS_ACTIVE = -1;
};

/**
 * Parameters that influence chain consensus.
 *
 **影响链条共识的参数
 */
struct Params {
    uint256 hashGenesisBlock;               //创世区块哈希
    int nSubsidyHalvingInterval;            //区块奖励减半的时间间隔
    /** Block height at which BIP16 becomes active */
    int BIP16Height;                        //区块高度
    /** Block height and hash at which BIP34 becomes active */
    int BIP34Height;                        
    uint256 BIP34Hash;                      //区块重量
    /** Block height at which BIP65 becomes active */
    int BIP65Height;
    /** Block height at which BIP66 becomes active */
    int BIP66Height;
    /**
     * Minimum blocks including miner confirmation of the total of 2016 blocks in a retargeting period,
     * (nPowTargetTimespan / nPowTargetSpacing) which is also used for BIP9 deployments.
     * Examples: 1916 for 95%, 1512 for testchains.
     *
     **在2016个区块中至少要有多少个区块被矿工认可，规则改变才生效
     * 在BIP9部署时还是使用(nPowTargetTimespan / nPowTargetSpacing)
     * eg:1916 for 95%, 1512 for 测试链
     */
    uint32_t nRuleChangeActivationThreshold;
    uint32_t nMinerConfirmationWindow;
    BIP9Deployment vDeployments[MAX_VERSION_BITS_DEPLOYMENTS];
    /** Proof of work parameters */
    /**POW参数*/
    uint256 powLimit;                       //难度
    bool fPowAllowMinDifficultyBlocks;      //是否允许最低难度
    bool fPowNoRetargeting;                 //不调整难度
    int64_t nPowTargetSpacing;              //区块产生平均时间
    int64_t nPowTargetTimespan;             //难度调整时间  10
    //难度调整的比例
    int64_t DifficultyAdjustmentInterval() const { return nPowTargetTimespan / nPowTargetSpacing; }
    uint256 nMinimumChainWork;              //当前难度调整最小值
    uint256 defaultAssumeValid;             //再次区块之前的区块都认为是有效的
};
} // namespace Consensus

```

### Pow

- ___源码路径:.../src/pow.cpp___

```C++
//获取下一次需要的工作量量
unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    //工作量证明的限制值
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // Only change once per difficulty adjustment interval
    // 难度发生调整时才有所改变
    if ((pindexLast->nHeight+1) % params.DifficultyAdjustmentInterval() != 0)
    {
        // 判断是否允许最小难度值
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            /**测试网络的特殊难度规则：
             * 如果新区块的时间戳超过两个区块的产生时间20min,那么返回限制挖矿难度
            */
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        //返回新区快的nBits值
        return pindexLast->nBits;
    }

    // Go back by what we want to be 14 days worth of blocks
    int nHeightFirst = pindexLast->nHeight - (params.DifficultyAdjustmentInterval()-1);
    assert(nHeightFirst >= 0);
    const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
    assert(pindexFirst);

    //计算工作量证明
    return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
}

//计算工作量证明
unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{   
    //如果不调整难度值
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    // Limit adjustment step
    // 难度调整的限制
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // Retarget
    //计算调整后的难度值
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

//检验工作量证明
bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}

```

### 比特币的共识机制(Pow)

每一个链上的区块在加入区块链之前都做了大量的计算，因此要想修改某一块的某些交易数据的成本是极其大的。这也是比特币不可篡改特性的一个重要技术依据。

工作量证明本质是在计算一个符合系统设定条件的哈希值，这个哈希值是通过对区块头信息进行哈希得到的。为了得到这个符合条件的哈希值，必须创建一个不超过特定值的区块块头的哈希。例如，如果最大可能的散列值是2 <sup>256</sup>  - 1，则可以通过产生小于2 <sup>255</sup>的散列值来证明您尝试了两种组合。

如果新的区块的哈希值符合共识协议期望的目标难度值条件，那么只会将新区块添加到区块链中。每2016 个块网络使用存储在每个区块头中的时间戳 来计算在生成最后2016 个区块的第一个和最后一个之间所经过的秒数。理想值是1209600秒（两周）。

*   如果生成2016 个区块的时间少于两个星期，则预期难度值会按比例增加（最多达300％），以便下一个2016 [个块]应该花费两周时间，以便以相同速率检查哈希值。

*   如果花费两个多星期来生成这些区块，出于同样的原因，预期的 难度值会成比例地下降（最高达75％）。












