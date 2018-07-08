// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_BLOCK_H
#define BITCOIN_PRIMITIVES_BLOCK_H

#include <primitives/transaction.h>
#include <serialize.h>
#include <uint256.h>

/** Nodes collect new transactions into a block, hash them into a hash tree,
 * and scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements.  When they solve the proof-of-work, they broadcast the block
 * to everyone and the block is added to the block chain.  The first transaction
 * in the block is a special one that creates a new coin owned by the creator
 * of the block.
 *
 **网络中的节点不断收集新的交易打包到区块中，所有的交易会通过两两哈希的方式形成一个Merkle树
 * 打包的过程就是要完成工作量证明的要求，当节点解出了当前的随机数时，
 * 它就把当前的区块广播到其他所有节点，并且加到区块链上。
 * 区块中的第一笔交易称之为CoinBase交易，是产生的新币，奖励给区块的产生者  
 * 
 * add by chaors 20180419
 */

//区块头
class CBlockHeader
{
public:
    // header
    int32_t nVersion;       //版本
    uint256 hashPrevBlock;  //上一个区块的hash
    uint256 hashMerkleRoot; //包含交易信息的Merkle树根
    uint32_t nTime;         //时间戳
    uint32_t nBits;         //工作量证明(POW)的难度
    uint32_t nNonce;        //要找的符合POW的随机数

    CBlockHeader()          //构造函数初始化成员变量
    {
        SetNull();          
    }

    ADD_SERIALIZE_METHODS;  //通过封装的模板实现类的序列化

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(this->nVersion);
        READWRITE(hashPrevBlock);
        READWRITE(hashMerkleRoot);
        READWRITE(nTime);
        READWRITE(nBits);
        READWRITE(nNonce);
    }

    void SetNull()          //初始化成员变量
    {
        nVersion = 0;
        hashPrevBlock.SetNull();
        hashMerkleRoot.SetNull();
        nTime = 0;
        nBits = 0;
        nNonce = 0;
    }

    bool IsNull() const
    {
        return (nBits == 0);     //难度为0说明区块还未创建，区块头为空
    }

    uint256 GetHash() const;     //获取哈希

    int64_t GetBlockTime() const //获取区块时间
    {
        return (int64_t)nTime;
    }
};


class CBlock : public CBlockHeader         //继承自CBlockHeader，拥有其所有成员变量
{
public:
    // network and disk
    std::vector<CTransactionRef> vtx;      //所有交易的容器

    // memory only
    mutable bool fChecked;                 //交易是否验证

    CBlock()
    {
        SetNull();
    }

    CBlock(const CBlockHeader &header)
    {
        SetNull();
        *((CBlockHeader*)this) = header;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(*(CBlockHeader*)this);
        READWRITE(vtx);
    }

    void SetNull()
    {
        CBlockHeader::SetNull();
        vtx.clear();
        fChecked = false;
    }

    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        block.nVersion       = nVersion;
        block.hashPrevBlock  = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nTime          = nTime;
        block.nBits          = nBits;
        block.nNonce         = nNonce;
        return block;
    }

    std::string ToString() const;
};

/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 *
 **描述区块链中在其他节点的一个位置，
 *如果其他节点没有相同的分支，它可以找到一个最近的中继(最近的相同块)。
 *更进一步地讲，它可能是分叉前的一个位置
 */
struct CBlockLocator
{
    std::vector<uint256> vHave;

    CBlockLocator() {}

    explicit CBlockLocator(const std::vector<uint256>& vHaveIn) : vHave(vHaveIn) {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        int nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vHave);
    }

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull() const
    {
        return vHave.empty();
    }
};

#endif // BITCOIN_PRIMITIVES_BLOCK_H
