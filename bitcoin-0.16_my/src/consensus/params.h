// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_PARAMS_H
#define BITCOIN_CONSENSUS_PARAMS_H

#include <uint256.h>
#include <limits>
#include <map>
#include <string>

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

#endif // BITCOIN_CONSENSUS_PARAMS_H
