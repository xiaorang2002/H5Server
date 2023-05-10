﻿#include "MjAlgorithm.h"

//牌型检测器列表 索引越低，检测优先级越高
const vector<ComposeTypeChecker> composeTypeCheckers =
{
    { { dasixi,sansanyi3n2, u8"大四喜", 88, { pengpenghu, dasanfeng, xiaosanfeng, xiaosixi, sizike } }, isDaSiXi },
    { { dasanyuan,sansanyi3n2, u8"大三元", 88, { shuangjianke, jianke } }, isDaSanYuan },
    { { jiulianbaodeng,sansanyi3n2, u8"九莲宝灯", 88, { qingyise, menqianqing, zimo, dandiaojiang, bianzhang, kanzhang } }, isJiuLianBaoDeng },
    { { dayuwu,otherhu000, u8"大于五", 88, { qingyise, lianqidui, sanyuanqiduizi, sixiqiduizi, qiduizi } }, isDaYuWu },
    { { xiaoyuwu,otherhu000, u8"小于五", 88, { qingyise, lianqidui, sanyuanqiduizi, sixiqiduizi, qiduizi } }, isXiaoYuWu },
    { { daqixing,qiduizi222,  u8"大七星", 88, { qiduizi, lianqidui, sanyuanqiduizi, sixiqiduizi, quandaiyao, dandiaojiang, menqianqing, zimo, ziyise, hunyaojiu } }, isDaQiXing },
    { { sigang,sansanyi3n2, u8"四杠", 88, { sangang, shuangminggang, dandiaojiang, minggang } }, isSiGang },
    { { lianqidui,qiduizi222, u8"连七对", 88, { qiduizi, sanyuanqiduizi, sixiqiduizi, dandiaojiang, menqianqing, zimo, qingyise, yibangao, lianliu, laoshaofu, pinghu } }, isLianQiDui },
    { { tianhu,otherhu000, u8"天胡", 88, { bianzhang, kanzhang, dandiaojiang, buqiuren, hujuezhang, zimo } }, isTianHu },
    { { dihu,otherhu000, u8"地胡", 88, {  } }, isDiHu },
    { { xiaosixi,sansanyi3n2, u8"小四喜", 64, { dasanfeng, xiaosanfeng } }, isXiaoSiXi },
    { { xiaosanyuan,sansanyi3n2, u8"小三元", 64, { shuangjianke, jianke } }, isXiaoSanYuan },
    { { sianke, sansanyi3n2,u8"四暗刻", 64, { sananke, shuanganke, pengpenghu, menqianqing, zimo } }, isSiAnKe },
    { { shuanglonghui,sansanyi3n2, u8"双龙会", 64, { yibangao, qiduizi, lianqidui, sanyuanqiduizi, sixiqiduizi, qingyise, laoshaofu, pinghu } }, isShuangLongHui },
    { { ziyise,otherhu000, u8"字一色", 64, { pengpenghu, quandaiyao, hunyaojiu, sizike } }, isZiYiSe },
    { { renhu, otherhu000,u8"人胡", 64, {  } }, isRenHu },
    { { sitongshun, sansanyi3n2,u8"四同顺", 48, { sanlianke, sananke, santongshun, qiduizi, lianqidui, sanyuanqiduizi, sixiqiduizi, siguiyi, yibangao, sananke } }, isSiTongShun },
    { { sanyuanqiduizi,qiduizi222,u8"三元七对子", 48, { qiduizi, menqianqing, dandiaojiang, zimo, yibangao, lianliu, laoshaofu } }, isSanYuanQiDui },
    { { sixiqiduizi, qiduizi222,u8"四喜七对子", 48, { qiduizi, menqianqing, dandiaojiang, zimo, yibangao, lianliu, laoshaofu } }, isSiXiQiDui },
    { { silianke, sansanyi3n2,u8"四连刻", 48, { sanlianke, santongshun, pengpenghu, yibangao } }, isSiLianKe },
    { { sibugao, sansanyi3n2,u8"四步高", 32, { sanbugao, lianliu, laoshaofu } }, isSiBuGao },
    { { hunyaojiu,otherhu000, u8"混幺九", 32, { pengpenghu, quandaiyao,qiduizi } }, isHunYaoJiu },
    { { sangang, sansanyi3n2,u8"三杠", 32, { shuangminggang, minggang } }, isSanGang },
    { { tianting,otherhu000, u8"天听", 32, { baoting } }, isTianTing },
    { { sizike, sansanyi3n2,u8"四字刻", 24, { pengpenghu } }, isSiZiKe },
    { { dasanfeng, sansanyi3n2,u8"大三风", 24, { xiaosanfeng } }, isDaSanFeng },
    { { sanlianke, sansanyi3n2,u8"三连刻", 24, { santongshun, yibangao } }, isSanLianKe },
    { { santongshun, sansanyi3n2,u8"三同顺", 24, { sanlianke, yibangao } }, isSanTongShun },
    { { qiduizi, qiduizi222,u8"七对子", 24, { menqianqing, dandiaojiang, zimo, yibangao, lianliu, laoshaofu } }, isQiDui },
    { { qinglong, sansanyi3n2,u8"清龙", 16, { lianliu, laoshaofu } }, isQingLong },
    { { sanbugao, sansanyi3n2,u8"三步高", 16, {  } }, isSanBuGao },
    { { sananke, sansanyi3n2,u8"三暗刻", 16, { shuanganke } }, isSanAnKe },
    { { quanhua, otherhu000,u8"全花", 16, { huapai } }, isQuanHua },
    { { qingyise, otherhu000,u8"清一色", 16, {  } }, isQingYiSe },
    { { miaoshouhuichun,otherhu000, u8"妙手回春", 8, { zimo } }, isMiaoShouHuiChun },
    { { haidilaoyue,otherhu000, u8"海底捞月", 8, {  } }, isHaiDiLaoYue },
    { { gangshangkaihua,otherhu000, u8"杠上开花", 8, { zimo } }, isGangShangKaiHua },
    { { qiangganghu,otherhu000, u8"抢杠胡", 8, { hujuezhang } }, isQiangGangHu },
    { { xiaosanfeng,sansanyi3n2, u8"小三风", 6, {  } }, isXiaoSanFeng },
    { { shuangjianke,sansanyi3n2, u8"双箭刻", 6, { jianke } }, isShuangJianKe },
    { { pengpenghu, sansanyi3n2,u8"碰碰和", 6, {  } }, isPengPengHu },
    { { shuangangang,sansanyi3n2, u8"双暗杠", 6, { shuanganke, angang } }, isShuangAnGang },
    { { hunyise, otherhu000,u8"混一色", 6, {  } }, isHunYiSe },
    { { quanqiuren,otherhu000, u8"全求人", 6, { dandiaojiang } }, isQuanQiuRen },
    { { quandaiyao,otherhu000, u8"全带幺", 4, {  } }, isQuanDaiYao },
    { { shuangminggang,sansanyi3n2, u8"双明杠", 4, { minggang } }, isShuangMingGang },
    { { buqiuren, otherhu000,u8"不求人", 4, { zimo, menqianqing } }, isBuQiuRen },
    { { hujuezhang,otherhu000, u8"和绝张", 4, {  } }, isHuJueZhang },
    { { jianke, sansanyi3n2,u8"箭刻", 2, {  } }, isJianKe },
    { { pinghu, sansanyi3n2,u8"平和", 2, {  } }, isPingHu },
    { { siguiyi, otherhu000, u8"四归一", 2, {  } }, isSiGuiYi },
    { { duanyao,otherhu000, u8"断幺", 2, {  } }, isDuanYao },
    { { shuanganke,sansanyi3n2, u8"双暗刻", 2, {  } }, isShuangAnKe },
    { { angang, sansanyi3n2,u8"暗杠", 2, {  } }, isAnGang },
    { { menqianqing, otherhu000,u8"门前清", 2, {  } }, isMenQianQing },
    { { baoting, otherhu000,u8"报听", 2, {  } }, isBaoTing },
    { { yibangao, sansanyi3n2,u8"一般高", 1, {  } }, isYiBanGao },
    { { lianliu, sansanyi3n2,u8"连六", 1, {  } }, isLianLiu },
    { { laoshaofu,sansanyi3n2, u8"老少副", 1, {  } }, isLaoShaoFu },
    { { minggang, sansanyi3n2,u8"明杠", 1, {  } }, isMingGang },
    { { dandiaojiang, otherhu000,u8"单钓将", 1, {  } }, isDanDiaoJiang },
    { { huapai,otherhu000, u8"花牌", 1, {  } }, isHuaPai },
    { { zimo, otherhu000,u8"自摸", 1, {  } }, isZiMo },
    { { bianzhang,sansanyi3n2, u8"边张", 1, {  } }, isBianZhang },
    { { kanzhang, sansanyi3n2,u8"砍张", 1, {  } }, isKanZhang }
};