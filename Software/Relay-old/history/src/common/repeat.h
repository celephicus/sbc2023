#ifndef __REPEAT_H_
#define __REPEAT_H_

// From https://gist.github.com/renaudcerrato. 

// Maximal number of repetitions supported by REPEAT
#define REPEAT_LIMIT    256

// Stringizing helper
#define __PASTE(a, b)   a##b
#define PASTE(a, b)     __PASTE(a, b)

/** 
  Repeat Macro:
    REPEAT(count, macro, data) 
    // expands to: 
    macro(0, data) macro(1, data) ... macro(count - 1, data)
  Example:
    #define FOO(n, data)  printf(data, n);
    REPEAT(3, FOO, "Hello %d!")
*/
#define REPEAT(count, macro, data) PASTE(REPEAT, count) (macro, data)

#define REPEAT0(macro, data)
#define REPEAT1(macro, data)      REPEAT0(macro, data)     macro(0, data)
#define REPEAT2(macro, data)      REPEAT1(macro, data)     macro(1, data)
#define REPEAT3(macro, data)      REPEAT2(macro, data)     macro(2, data)
#define REPEAT4(macro, data)      REPEAT3(macro, data)     macro(3, data)
#define REPEAT5(macro, data)      REPEAT4(macro, data)     macro(4, data)
#define REPEAT6(macro, data)      REPEAT5(macro, data)     macro(5, data)
#define REPEAT7(macro, data)      REPEAT6(macro, data)     macro(6, data)
#define REPEAT8(macro, data)      REPEAT7(macro, data)     macro(7, data)
#define REPEAT9(macro, data)      REPEAT8(macro, data)     macro(8, data)
#define REPEAT10(macro, data)     REPEAT9(macro, data)     macro(9, data)
#define REPEAT11(macro, data)     REPEAT10(macro, data)    macro(10, data)
#define REPEAT12(macro, data)     REPEAT11(macro, data)    macro(11, data)
#define REPEAT13(macro, data)     REPEAT12(macro, data)    macro(12, data)
#define REPEAT14(macro, data)     REPEAT13(macro, data)    macro(13, data)
#define REPEAT15(macro, data)     REPEAT14(macro, data)    macro(14, data)
#define REPEAT16(macro, data)     REPEAT15(macro, data)    macro(15, data)
#define REPEAT17(macro, data)     REPEAT16(macro, data)    macro(16, data)
#define REPEAT18(macro, data)     REPEAT17(macro, data)    macro(17, data)
#define REPEAT19(macro, data)     REPEAT18(macro, data)    macro(18, data)
#define REPEAT20(macro, data)     REPEAT19(macro, data)    macro(19, data)
#define REPEAT21(macro, data)     REPEAT20(macro, data)    macro(20, data)
#define REPEAT22(macro, data)     REPEAT21(macro, data)    macro(21, data)
#define REPEAT23(macro, data)     REPEAT22(macro, data)    macro(22, data)
#define REPEAT24(macro, data)     REPEAT23(macro, data)    macro(23, data)
#define REPEAT25(macro, data)     REPEAT24(macro, data)    macro(24, data)
#define REPEAT26(macro, data)     REPEAT25(macro, data)    macro(25, data)
#define REPEAT27(macro, data)     REPEAT26(macro, data)    macro(26, data)
#define REPEAT28(macro, data)     REPEAT27(macro, data)    macro(27, data)
#define REPEAT29(macro, data)     REPEAT28(macro, data)    macro(28, data)
#define REPEAT30(macro, data)     REPEAT29(macro, data)    macro(29, data)
#define REPEAT31(macro, data)     REPEAT30(macro, data)    macro(30, data)
#define REPEAT32(macro, data)     REPEAT31(macro, data)    macro(31, data)
#define REPEAT33(macro, data)     REPEAT32(macro, data)    macro(32, data)
#define REPEAT34(macro, data)     REPEAT33(macro, data)    macro(33, data)
#define REPEAT35(macro, data)     REPEAT34(macro, data)    macro(34, data)
#define REPEAT36(macro, data)     REPEAT35(macro, data)    macro(35, data)
#define REPEAT37(macro, data)     REPEAT36(macro, data)    macro(36, data)
#define REPEAT38(macro, data)     REPEAT37(macro, data)    macro(37, data)
#define REPEAT39(macro, data)     REPEAT38(macro, data)    macro(38, data)
#define REPEAT40(macro, data)     REPEAT39(macro, data)    macro(39, data)
#define REPEAT41(macro, data)     REPEAT40(macro, data)    macro(40, data)
#define REPEAT42(macro, data)     REPEAT41(macro, data)    macro(41, data)
#define REPEAT43(macro, data)     REPEAT42(macro, data)    macro(42, data)
#define REPEAT44(macro, data)     REPEAT43(macro, data)    macro(43, data)
#define REPEAT45(macro, data)     REPEAT44(macro, data)    macro(44, data)
#define REPEAT46(macro, data)     REPEAT45(macro, data)    macro(45, data)
#define REPEAT47(macro, data)     REPEAT46(macro, data)    macro(46, data)
#define REPEAT48(macro, data)     REPEAT47(macro, data)    macro(47, data)
#define REPEAT49(macro, data)     REPEAT48(macro, data)    macro(48, data)
#define REPEAT50(macro, data)     REPEAT49(macro, data)    macro(49, data)
#define REPEAT51(macro, data)     REPEAT50(macro, data)    macro(50, data)
#define REPEAT52(macro, data)     REPEAT51(macro, data)    macro(51, data)
#define REPEAT53(macro, data)     REPEAT52(macro, data)    macro(52, data)
#define REPEAT54(macro, data)     REPEAT53(macro, data)    macro(53, data)
#define REPEAT55(macro, data)     REPEAT54(macro, data)    macro(54, data)
#define REPEAT56(macro, data)     REPEAT55(macro, data)    macro(55, data)
#define REPEAT57(macro, data)     REPEAT56(macro, data)    macro(56, data)
#define REPEAT58(macro, data)     REPEAT57(macro, data)    macro(57, data)
#define REPEAT59(macro, data)     REPEAT58(macro, data)    macro(58, data)
#define REPEAT60(macro, data)     REPEAT59(macro, data)    macro(59, data)
#define REPEAT61(macro, data)     REPEAT60(macro, data)    macro(60, data)
#define REPEAT62(macro, data)     REPEAT61(macro, data)    macro(61, data)
#define REPEAT63(macro, data)     REPEAT62(macro, data)    macro(62, data)
#define REPEAT64(macro, data)     REPEAT63(macro, data)    macro(63, data)
#define REPEAT65(macro, data)     REPEAT64(macro, data)    macro(64, data)
#define REPEAT66(macro, data)     REPEAT65(macro, data)    macro(65, data)
#define REPEAT67(macro, data)     REPEAT66(macro, data)    macro(66, data)
#define REPEAT68(macro, data)     REPEAT67(macro, data)    macro(67, data)
#define REPEAT69(macro, data)     REPEAT68(macro, data)    macro(68, data)
#define REPEAT70(macro, data)     REPEAT69(macro, data)    macro(69, data)
#define REPEAT71(macro, data)     REPEAT70(macro, data)    macro(70, data)
#define REPEAT72(macro, data)     REPEAT71(macro, data)    macro(71, data)
#define REPEAT73(macro, data)     REPEAT72(macro, data)    macro(72, data)
#define REPEAT74(macro, data)     REPEAT73(macro, data)    macro(73, data)
#define REPEAT75(macro, data)     REPEAT74(macro, data)    macro(74, data)
#define REPEAT76(macro, data)     REPEAT75(macro, data)    macro(75, data)
#define REPEAT77(macro, data)     REPEAT76(macro, data)    macro(76, data)
#define REPEAT78(macro, data)     REPEAT77(macro, data)    macro(77, data)
#define REPEAT79(macro, data)     REPEAT78(macro, data)    macro(78, data)
#define REPEAT80(macro, data)     REPEAT79(macro, data)    macro(79, data)
#define REPEAT81(macro, data)     REPEAT80(macro, data)    macro(80, data)
#define REPEAT82(macro, data)     REPEAT81(macro, data)    macro(81, data)
#define REPEAT83(macro, data)     REPEAT82(macro, data)    macro(82, data)
#define REPEAT84(macro, data)     REPEAT83(macro, data)    macro(83, data)
#define REPEAT85(macro, data)     REPEAT84(macro, data)    macro(84, data)
#define REPEAT86(macro, data)     REPEAT85(macro, data)    macro(85, data)
#define REPEAT87(macro, data)     REPEAT86(macro, data)    macro(86, data)
#define REPEAT88(macro, data)     REPEAT87(macro, data)    macro(87, data)
#define REPEAT89(macro, data)     REPEAT88(macro, data)    macro(88, data)
#define REPEAT90(macro, data)     REPEAT89(macro, data)    macro(89, data)
#define REPEAT91(macro, data)     REPEAT90(macro, data)    macro(90, data)
#define REPEAT92(macro, data)     REPEAT91(macro, data)    macro(91, data)
#define REPEAT93(macro, data)     REPEAT92(macro, data)    macro(92, data)
#define REPEAT94(macro, data)     REPEAT93(macro, data)    macro(93, data)
#define REPEAT95(macro, data)     REPEAT94(macro, data)    macro(94, data)
#define REPEAT96(macro, data)     REPEAT95(macro, data)    macro(95, data)
#define REPEAT97(macro, data)     REPEAT96(macro, data)    macro(96, data)
#define REPEAT98(macro, data)     REPEAT97(macro, data)    macro(97, data)
#define REPEAT99(macro, data)     REPEAT98(macro, data)    macro(98, data)
#define REPEAT100(macro, data)    REPEAT99(macro, data)    macro(99, data)
#define REPEAT101(macro, data)    REPEAT100(macro, data)   macro(100, data)
#define REPEAT102(macro, data)    REPEAT101(macro, data)   macro(101, data)
#define REPEAT103(macro, data)    REPEAT102(macro, data)   macro(102, data)
#define REPEAT104(macro, data)    REPEAT103(macro, data)   macro(103, data)
#define REPEAT105(macro, data)    REPEAT104(macro, data)   macro(104, data)
#define REPEAT106(macro, data)    REPEAT105(macro, data)   macro(105, data)
#define REPEAT107(macro, data)    REPEAT106(macro, data)   macro(106, data)
#define REPEAT108(macro, data)    REPEAT107(macro, data)   macro(107, data)
#define REPEAT109(macro, data)    REPEAT108(macro, data)   macro(108, data)
#define REPEAT110(macro, data)    REPEAT109(macro, data)   macro(109, data)
#define REPEAT111(macro, data)    REPEAT110(macro, data)   macro(110, data)
#define REPEAT112(macro, data)    REPEAT111(macro, data)   macro(111, data)
#define REPEAT113(macro, data)    REPEAT112(macro, data)   macro(112, data)
#define REPEAT114(macro, data)    REPEAT113(macro, data)   macro(113, data)
#define REPEAT115(macro, data)    REPEAT114(macro, data)   macro(114, data)
#define REPEAT116(macro, data)    REPEAT115(macro, data)   macro(115, data)
#define REPEAT117(macro, data)    REPEAT116(macro, data)   macro(116, data)
#define REPEAT118(macro, data)    REPEAT117(macro, data)   macro(117, data)
#define REPEAT119(macro, data)    REPEAT118(macro, data)   macro(118, data)
#define REPEAT120(macro, data)    REPEAT119(macro, data)   macro(119, data)
#define REPEAT121(macro, data)    REPEAT120(macro, data)   macro(120, data)
#define REPEAT122(macro, data)    REPEAT121(macro, data)   macro(121, data)
#define REPEAT123(macro, data)    REPEAT122(macro, data)   macro(122, data)
#define REPEAT124(macro, data)    REPEAT123(macro, data)   macro(123, data)
#define REPEAT125(macro, data)    REPEAT124(macro, data)   macro(124, data)
#define REPEAT126(macro, data)    REPEAT125(macro, data)   macro(125, data)
#define REPEAT127(macro, data)    REPEAT126(macro, data)   macro(126, data)
#define REPEAT128(macro, data)    REPEAT127(macro, data)   macro(127, data)
#define REPEAT129(macro, data)    REPEAT128(macro, data)   macro(128, data)
#define REPEAT130(macro, data)    REPEAT129(macro, data)   macro(129, data)
#define REPEAT131(macro, data)    REPEAT130(macro, data)   macro(130, data)
#define REPEAT132(macro, data)    REPEAT131(macro, data)   macro(131, data)
#define REPEAT133(macro, data)    REPEAT132(macro, data)   macro(132, data)
#define REPEAT134(macro, data)    REPEAT133(macro, data)   macro(133, data)
#define REPEAT135(macro, data)    REPEAT134(macro, data)   macro(134, data)
#define REPEAT136(macro, data)    REPEAT135(macro, data)   macro(135, data)
#define REPEAT137(macro, data)    REPEAT136(macro, data)   macro(136, data)
#define REPEAT138(macro, data)    REPEAT137(macro, data)   macro(137, data)
#define REPEAT139(macro, data)    REPEAT138(macro, data)   macro(138, data)
#define REPEAT140(macro, data)    REPEAT139(macro, data)   macro(139, data)
#define REPEAT141(macro, data)    REPEAT140(macro, data)   macro(140, data)
#define REPEAT142(macro, data)    REPEAT141(macro, data)   macro(141, data)
#define REPEAT143(macro, data)    REPEAT142(macro, data)   macro(142, data)
#define REPEAT144(macro, data)    REPEAT143(macro, data)   macro(143, data)
#define REPEAT145(macro, data)    REPEAT144(macro, data)   macro(144, data)
#define REPEAT146(macro, data)    REPEAT145(macro, data)   macro(145, data)
#define REPEAT147(macro, data)    REPEAT146(macro, data)   macro(146, data)
#define REPEAT148(macro, data)    REPEAT147(macro, data)   macro(147, data)
#define REPEAT149(macro, data)    REPEAT148(macro, data)   macro(148, data)
#define REPEAT150(macro, data)    REPEAT149(macro, data)   macro(149, data)
#define REPEAT151(macro, data)    REPEAT150(macro, data)   macro(150, data)
#define REPEAT152(macro, data)    REPEAT151(macro, data)   macro(151, data)
#define REPEAT153(macro, data)    REPEAT152(macro, data)   macro(152, data)
#define REPEAT154(macro, data)    REPEAT153(macro, data)   macro(153, data)
#define REPEAT155(macro, data)    REPEAT154(macro, data)   macro(154, data)
#define REPEAT156(macro, data)    REPEAT155(macro, data)   macro(155, data)
#define REPEAT157(macro, data)    REPEAT156(macro, data)   macro(156, data)
#define REPEAT158(macro, data)    REPEAT157(macro, data)   macro(157, data)
#define REPEAT159(macro, data)    REPEAT158(macro, data)   macro(158, data)
#define REPEAT160(macro, data)    REPEAT159(macro, data)   macro(159, data)
#define REPEAT161(macro, data)    REPEAT160(macro, data)   macro(160, data)
#define REPEAT162(macro, data)    REPEAT161(macro, data)   macro(161, data)
#define REPEAT163(macro, data)    REPEAT162(macro, data)   macro(162, data)
#define REPEAT164(macro, data)    REPEAT163(macro, data)   macro(163, data)
#define REPEAT165(macro, data)    REPEAT164(macro, data)   macro(164, data)
#define REPEAT166(macro, data)    REPEAT165(macro, data)   macro(165, data)
#define REPEAT167(macro, data)    REPEAT166(macro, data)   macro(166, data)
#define REPEAT168(macro, data)    REPEAT167(macro, data)   macro(167, data)
#define REPEAT169(macro, data)    REPEAT168(macro, data)   macro(168, data)
#define REPEAT170(macro, data)    REPEAT169(macro, data)   macro(169, data)
#define REPEAT171(macro, data)    REPEAT170(macro, data)   macro(170, data)
#define REPEAT172(macro, data)    REPEAT171(macro, data)   macro(171, data)
#define REPEAT173(macro, data)    REPEAT172(macro, data)   macro(172, data)
#define REPEAT174(macro, data)    REPEAT173(macro, data)   macro(173, data)
#define REPEAT175(macro, data)    REPEAT174(macro, data)   macro(174, data)
#define REPEAT176(macro, data)    REPEAT175(macro, data)   macro(175, data)
#define REPEAT177(macro, data)    REPEAT176(macro, data)   macro(176, data)
#define REPEAT178(macro, data)    REPEAT177(macro, data)   macro(177, data)
#define REPEAT179(macro, data)    REPEAT178(macro, data)   macro(178, data)
#define REPEAT180(macro, data)    REPEAT179(macro, data)   macro(179, data)
#define REPEAT181(macro, data)    REPEAT180(macro, data)   macro(180, data)
#define REPEAT182(macro, data)    REPEAT181(macro, data)   macro(181, data)
#define REPEAT183(macro, data)    REPEAT182(macro, data)   macro(182, data)
#define REPEAT184(macro, data)    REPEAT183(macro, data)   macro(183, data)
#define REPEAT185(macro, data)    REPEAT184(macro, data)   macro(184, data)
#define REPEAT186(macro, data)    REPEAT185(macro, data)   macro(185, data)
#define REPEAT187(macro, data)    REPEAT186(macro, data)   macro(186, data)
#define REPEAT188(macro, data)    REPEAT187(macro, data)   macro(187, data)
#define REPEAT189(macro, data)    REPEAT188(macro, data)   macro(188, data)
#define REPEAT190(macro, data)    REPEAT189(macro, data)   macro(189, data)
#define REPEAT191(macro, data)    REPEAT190(macro, data)   macro(190, data)
#define REPEAT192(macro, data)    REPEAT191(macro, data)   macro(191, data)
#define REPEAT193(macro, data)    REPEAT192(macro, data)   macro(192, data)
#define REPEAT194(macro, data)    REPEAT193(macro, data)   macro(193, data)
#define REPEAT195(macro, data)    REPEAT194(macro, data)   macro(194, data)
#define REPEAT196(macro, data)    REPEAT195(macro, data)   macro(195, data)
#define REPEAT197(macro, data)    REPEAT196(macro, data)   macro(196, data)
#define REPEAT198(macro, data)    REPEAT197(macro, data)   macro(197, data)
#define REPEAT199(macro, data)    REPEAT198(macro, data)   macro(198, data)
#define REPEAT200(macro, data)    REPEAT199(macro, data)   macro(199, data)
#define REPEAT201(macro, data)    REPEAT200(macro, data)   macro(200, data)
#define REPEAT202(macro, data)    REPEAT201(macro, data)   macro(201, data)
#define REPEAT203(macro, data)    REPEAT202(macro, data)   macro(202, data)
#define REPEAT204(macro, data)    REPEAT203(macro, data)   macro(203, data)
#define REPEAT205(macro, data)    REPEAT204(macro, data)   macro(204, data)
#define REPEAT206(macro, data)    REPEAT205(macro, data)   macro(205, data)
#define REPEAT207(macro, data)    REPEAT206(macro, data)   macro(206, data)
#define REPEAT208(macro, data)    REPEAT207(macro, data)   macro(207, data)
#define REPEAT209(macro, data)    REPEAT208(macro, data)   macro(208, data)
#define REPEAT210(macro, data)    REPEAT209(macro, data)   macro(209, data)
#define REPEAT211(macro, data)    REPEAT210(macro, data)   macro(210, data)
#define REPEAT212(macro, data)    REPEAT211(macro, data)   macro(211, data)
#define REPEAT213(macro, data)    REPEAT212(macro, data)   macro(212, data)
#define REPEAT214(macro, data)    REPEAT213(macro, data)   macro(213, data)
#define REPEAT215(macro, data)    REPEAT214(macro, data)   macro(214, data)
#define REPEAT216(macro, data)    REPEAT215(macro, data)   macro(215, data)
#define REPEAT217(macro, data)    REPEAT216(macro, data)   macro(216, data)
#define REPEAT218(macro, data)    REPEAT217(macro, data)   macro(217, data)
#define REPEAT219(macro, data)    REPEAT218(macro, data)   macro(218, data)
#define REPEAT220(macro, data)    REPEAT219(macro, data)   macro(219, data)
#define REPEAT221(macro, data)    REPEAT220(macro, data)   macro(220, data)
#define REPEAT222(macro, data)    REPEAT221(macro, data)   macro(221, data)
#define REPEAT223(macro, data)    REPEAT222(macro, data)   macro(222, data)
#define REPEAT224(macro, data)    REPEAT223(macro, data)   macro(223, data)
#define REPEAT225(macro, data)    REPEAT224(macro, data)   macro(224, data)
#define REPEAT226(macro, data)    REPEAT225(macro, data)   macro(225, data)
#define REPEAT227(macro, data)    REPEAT226(macro, data)   macro(226, data)
#define REPEAT228(macro, data)    REPEAT227(macro, data)   macro(227, data)
#define REPEAT229(macro, data)    REPEAT228(macro, data)   macro(228, data)
#define REPEAT230(macro, data)    REPEAT229(macro, data)   macro(229, data)
#define REPEAT231(macro, data)    REPEAT230(macro, data)   macro(230, data)
#define REPEAT232(macro, data)    REPEAT231(macro, data)   macro(231, data)
#define REPEAT233(macro, data)    REPEAT232(macro, data)   macro(232, data)
#define REPEAT234(macro, data)    REPEAT233(macro, data)   macro(233, data)
#define REPEAT235(macro, data)    REPEAT234(macro, data)   macro(234, data)
#define REPEAT236(macro, data)    REPEAT235(macro, data)   macro(235, data)
#define REPEAT237(macro, data)    REPEAT236(macro, data)   macro(236, data)
#define REPEAT238(macro, data)    REPEAT237(macro, data)   macro(237, data)
#define REPEAT239(macro, data)    REPEAT238(macro, data)   macro(238, data)
#define REPEAT240(macro, data)    REPEAT239(macro, data)   macro(239, data)
#define REPEAT241(macro, data)    REPEAT240(macro, data)   macro(240, data)
#define REPEAT242(macro, data)    REPEAT241(macro, data)   macro(241, data)
#define REPEAT243(macro, data)    REPEAT242(macro, data)   macro(242, data)
#define REPEAT244(macro, data)    REPEAT243(macro, data)   macro(243, data)
#define REPEAT245(macro, data)    REPEAT244(macro, data)   macro(244, data)
#define REPEAT246(macro, data)    REPEAT245(macro, data)   macro(245, data)
#define REPEAT247(macro, data)    REPEAT246(macro, data)   macro(246, data)
#define REPEAT248(macro, data)    REPEAT247(macro, data)   macro(247, data)
#define REPEAT249(macro, data)    REPEAT248(macro, data)   macro(248, data)
#define REPEAT250(macro, data)    REPEAT249(macro, data)   macro(249, data)
#define REPEAT251(macro, data)    REPEAT250(macro, data)   macro(250, data)
#define REPEAT252(macro, data)    REPEAT251(macro, data)   macro(251, data)
#define REPEAT253(macro, data)    REPEAT252(macro, data)   macro(252, data)
#define REPEAT254(macro, data)    REPEAT253(macro, data)   macro(253, data)
#define REPEAT255(macro, data)    REPEAT254(macro, data)   macro(254, data)
#define REPEAT256(macro, data)    REPEAT255(macro, data)   macro(255, data)

/**
  Recursive Repeat Macro:
  
    RREPEAT(count, macro, data) 
    // expands to: 
    macro(data-count+1, 0) macro(data-count+2, 1) ... macro(data, count-1)
*/
#define RREPEAT(count, macro, data) PASTE(RREPEAT, count) (macro, data)

#define RREPEAT0(macro, data)
#define RREPEAT1(macro, data)      RREPEAT0(macro, DEC_(data))     macro(data, 0)
#define RREPEAT2(macro, data)      RREPEAT1(macro, DEC_(data))     macro(data, 1)
#define RREPEAT3(macro, data)      RREPEAT2(macro, DEC_(data))     macro(data, 2)
#define RREPEAT4(macro, data)      RREPEAT3(macro, DEC_(data))     macro(data, 3)
#define RREPEAT5(macro, data)      RREPEAT4(macro, DEC_(data))     macro(data, 4)
#define RREPEAT6(macro, data)      RREPEAT5(macro, DEC_(data))     macro(data, 5)
#define RREPEAT7(macro, data)      RREPEAT6(macro, DEC_(data))     macro(data, 6)
#define RREPEAT8(macro, data)      RREPEAT7(macro, DEC_(data))     macro(data, 7)
#define RREPEAT9(macro, data)      RREPEAT8(macro, DEC_(data))     macro(data, 8)
#define RREPEAT10(macro, data)     RREPEAT9(macro, DEC_(data))     macro(data, 9)
#define RREPEAT11(macro, data)     RREPEAT10(macro, DEC_(data))    macro(data, 10)
#define RREPEAT12(macro, data)     RREPEAT11(macro, DEC_(data))    macro(data, 11)
#define RREPEAT13(macro, data)     RREPEAT12(macro, DEC_(data))    macro(data, 12)
#define RREPEAT14(macro, data)     RREPEAT13(macro, DEC_(data))    macro(data, 13)
#define RREPEAT15(macro, data)     RREPEAT14(macro, DEC_(data))    macro(data, 14)
#define RREPEAT16(macro, data)     RREPEAT15(macro, DEC_(data))    macro(data, 15)
#define RREPEAT17(macro, data)     RREPEAT16(macro, DEC_(data))    macro(data, 16)
#define RREPEAT18(macro, data)     RREPEAT17(macro, DEC_(data))    macro(data, 17)
#define RREPEAT19(macro, data)     RREPEAT18(macro, DEC_(data))    macro(data, 18)
#define RREPEAT20(macro, data)     RREPEAT19(macro, DEC_(data))    macro(data, 19)
#define RREPEAT21(macro, data)     RREPEAT20(macro, DEC_(data))    macro(data, 20)
#define RREPEAT22(macro, data)     RREPEAT21(macro, DEC_(data))    macro(data, 21)
#define RREPEAT23(macro, data)     RREPEAT22(macro, DEC_(data))    macro(data, 22)
#define RREPEAT24(macro, data)     RREPEAT23(macro, DEC_(data))    macro(data, 23)
#define RREPEAT25(macro, data)     RREPEAT24(macro, DEC_(data))    macro(data, 24)
#define RREPEAT26(macro, data)     RREPEAT25(macro, DEC_(data))    macro(data, 25)
#define RREPEAT27(macro, data)     RREPEAT26(macro, DEC_(data))    macro(data, 26)
#define RREPEAT28(macro, data)     RREPEAT27(macro, DEC_(data))    macro(data, 27)
#define RREPEAT29(macro, data)     RREPEAT28(macro, DEC_(data))    macro(data, 28)
#define RREPEAT30(macro, data)     RREPEAT29(macro, DEC_(data))    macro(data, 29)
#define RREPEAT31(macro, data)     RREPEAT30(macro, DEC_(data))    macro(data, 30)
#define RREPEAT32(macro, data)     RREPEAT31(macro, DEC_(data))    macro(data, 31)
#define RREPEAT33(macro, data)     RREPEAT32(macro, DEC_(data))    macro(data, 32)
#define RREPEAT34(macro, data)     RREPEAT33(macro, DEC_(data))    macro(data, 33)
#define RREPEAT35(macro, data)     RREPEAT34(macro, DEC_(data))    macro(data, 34)
#define RREPEAT36(macro, data)     RREPEAT35(macro, DEC_(data))    macro(data, 35)
#define RREPEAT37(macro, data)     RREPEAT36(macro, DEC_(data))    macro(data, 36)
#define RREPEAT38(macro, data)     RREPEAT37(macro, DEC_(data))    macro(data, 37)
#define RREPEAT39(macro, data)     RREPEAT38(macro, DEC_(data))    macro(data, 38)
#define RREPEAT40(macro, data)     RREPEAT39(macro, DEC_(data))    macro(data, 39)
#define RREPEAT41(macro, data)     RREPEAT40(macro, DEC_(data))    macro(data, 40)
#define RREPEAT42(macro, data)     RREPEAT41(macro, DEC_(data))    macro(data, 41)
#define RREPEAT43(macro, data)     RREPEAT42(macro, DEC_(data))    macro(data, 42)
#define RREPEAT44(macro, data)     RREPEAT43(macro, DEC_(data))    macro(data, 43)
#define RREPEAT45(macro, data)     RREPEAT44(macro, DEC_(data))    macro(data, 44)
#define RREPEAT46(macro, data)     RREPEAT45(macro, DEC_(data))    macro(data, 45)
#define RREPEAT47(macro, data)     RREPEAT46(macro, DEC_(data))    macro(data, 46)
#define RREPEAT48(macro, data)     RREPEAT47(macro, DEC_(data))    macro(data, 47)
#define RREPEAT49(macro, data)     RREPEAT48(macro, DEC_(data))    macro(data, 48)
#define RREPEAT50(macro, data)     RREPEAT49(macro, DEC_(data))    macro(data, 49)
#define RREPEAT51(macro, data)     RREPEAT50(macro, DEC_(data))    macro(data, 50)
#define RREPEAT52(macro, data)     RREPEAT51(macro, DEC_(data))    macro(data, 51)
#define RREPEAT53(macro, data)     RREPEAT52(macro, DEC_(data))    macro(data, 52)
#define RREPEAT54(macro, data)     RREPEAT53(macro, DEC_(data))    macro(data, 53)
#define RREPEAT55(macro, data)     RREPEAT54(macro, DEC_(data))    macro(data, 54)
#define RREPEAT56(macro, data)     RREPEAT55(macro, DEC_(data))    macro(data, 55)
#define RREPEAT57(macro, data)     RREPEAT56(macro, DEC_(data))    macro(data, 56)
#define RREPEAT58(macro, data)     RREPEAT57(macro, DEC_(data))    macro(data, 57)
#define RREPEAT59(macro, data)     RREPEAT58(macro, DEC_(data))    macro(data, 58)
#define RREPEAT60(macro, data)     RREPEAT59(macro, DEC_(data))    macro(data, 59)
#define RREPEAT61(macro, data)     RREPEAT60(macro, DEC_(data))    macro(data, 60)
#define RREPEAT62(macro, data)     RREPEAT61(macro, DEC_(data))    macro(data, 61)
#define RREPEAT63(macro, data)     RREPEAT62(macro, DEC_(data))    macro(data, 62)
#define RREPEAT64(macro, data)     RREPEAT63(macro, DEC_(data))    macro(data, 63)
#define RREPEAT65(macro, data)     RREPEAT64(macro, DEC_(data))    macro(data, 64)
#define RREPEAT66(macro, data)     RREPEAT65(macro, DEC_(data))    macro(data, 65)
#define RREPEAT67(macro, data)     RREPEAT66(macro, DEC_(data))    macro(data, 66)
#define RREPEAT68(macro, data)     RREPEAT67(macro, DEC_(data))    macro(data, 67)
#define RREPEAT69(macro, data)     RREPEAT68(macro, DEC_(data))    macro(data, 68)
#define RREPEAT70(macro, data)     RREPEAT69(macro, DEC_(data))    macro(data, 69)
#define RREPEAT71(macro, data)     RREPEAT70(macro, DEC_(data))    macro(data, 70)
#define RREPEAT72(macro, data)     RREPEAT71(macro, DEC_(data))    macro(data, 71)
#define RREPEAT73(macro, data)     RREPEAT72(macro, DEC_(data))    macro(data, 72)
#define RREPEAT74(macro, data)     RREPEAT73(macro, DEC_(data))    macro(data, 73)
#define RREPEAT75(macro, data)     RREPEAT74(macro, DEC_(data))    macro(data, 74)
#define RREPEAT76(macro, data)     RREPEAT75(macro, DEC_(data))    macro(data, 75)
#define RREPEAT77(macro, data)     RREPEAT76(macro, DEC_(data))    macro(data, 76)
#define RREPEAT78(macro, data)     RREPEAT77(macro, DEC_(data))    macro(data, 77)
#define RREPEAT79(macro, data)     RREPEAT78(macro, DEC_(data))    macro(data, 78)
#define RREPEAT80(macro, data)     RREPEAT79(macro, DEC_(data))    macro(data, 79)
#define RREPEAT81(macro, data)     RREPEAT80(macro, DEC_(data))    macro(data, 80)
#define RREPEAT82(macro, data)     RREPEAT81(macro, DEC_(data))    macro(data, 81)
#define RREPEAT83(macro, data)     RREPEAT82(macro, DEC_(data))    macro(data, 82)
#define RREPEAT84(macro, data)     RREPEAT83(macro, DEC_(data))    macro(data, 83)
#define RREPEAT85(macro, data)     RREPEAT84(macro, DEC_(data))    macro(data, 84)
#define RREPEAT86(macro, data)     RREPEAT85(macro, DEC_(data))    macro(data, 85)
#define RREPEAT87(macro, data)     RREPEAT86(macro, DEC_(data))    macro(data, 86)
#define RREPEAT88(macro, data)     RREPEAT87(macro, DEC_(data))    macro(data, 87)
#define RREPEAT89(macro, data)     RREPEAT88(macro, DEC_(data))    macro(data, 88)
#define RREPEAT90(macro, data)     RREPEAT89(macro, DEC_(data))    macro(data, 89)
#define RREPEAT91(macro, data)     RREPEAT90(macro, DEC_(data))    macro(data, 90)
#define RREPEAT92(macro, data)     RREPEAT91(macro, DEC_(data))    macro(data, 91)
#define RREPEAT93(macro, data)     RREPEAT92(macro, DEC_(data))    macro(data, 92)
#define RREPEAT94(macro, data)     RREPEAT93(macro, DEC_(data))    macro(data, 93)
#define RREPEAT95(macro, data)     RREPEAT94(macro, DEC_(data))    macro(data, 94)
#define RREPEAT96(macro, data)     RREPEAT95(macro, DEC_(data))    macro(data, 95)
#define RREPEAT97(macro, data)     RREPEAT96(macro, DEC_(data))    macro(data, 96)
#define RREPEAT98(macro, data)     RREPEAT97(macro, DEC_(data))    macro(data, 97)
#define RREPEAT99(macro, data)     RREPEAT98(macro, DEC_(data))    macro(data, 98)
#define RREPEAT100(macro, data)    RREPEAT99(macro, DEC_(data))    macro(data, 99)
#define RREPEAT101(macro, data)    RREPEAT100(macro, DEC_(data))   macro(data, 100)
#define RREPEAT102(macro, data)    RREPEAT101(macro, DEC_(data))   macro(data, 101)
#define RREPEAT103(macro, data)    RREPEAT102(macro, DEC_(data))   macro(data, 102)
#define RREPEAT104(macro, data)    RREPEAT103(macro, DEC_(data))   macro(data, 103)
#define RREPEAT105(macro, data)    RREPEAT104(macro, DEC_(data))   macro(data, 104)
#define RREPEAT106(macro, data)    RREPEAT105(macro, DEC_(data))   macro(data, 105)
#define RREPEAT107(macro, data)    RREPEAT106(macro, DEC_(data))   macro(data, 106)
#define RREPEAT108(macro, data)    RREPEAT107(macro, DEC_(data))   macro(data, 107)
#define RREPEAT109(macro, data)    RREPEAT108(macro, DEC_(data))   macro(data, 108)
#define RREPEAT110(macro, data)    RREPEAT109(macro, DEC_(data))   macro(data, 109)
#define RREPEAT111(macro, data)    RREPEAT110(macro, DEC_(data))   macro(data, 110)
#define RREPEAT112(macro, data)    RREPEAT111(macro, DEC_(data))   macro(data, 111)
#define RREPEAT113(macro, data)    RREPEAT112(macro, DEC_(data))   macro(data, 112)
#define RREPEAT114(macro, data)    RREPEAT113(macro, DEC_(data))   macro(data, 113)
#define RREPEAT115(macro, data)    RREPEAT114(macro, DEC_(data))   macro(data, 114)
#define RREPEAT116(macro, data)    RREPEAT115(macro, DEC_(data))   macro(data, 115)
#define RREPEAT117(macro, data)    RREPEAT116(macro, DEC_(data))   macro(data, 116)
#define RREPEAT118(macro, data)    RREPEAT117(macro, DEC_(data))   macro(data, 117)
#define RREPEAT119(macro, data)    RREPEAT118(macro, DEC_(data))   macro(data, 118)
#define RREPEAT120(macro, data)    RREPEAT119(macro, DEC_(data))   macro(data, 119)
#define RREPEAT121(macro, data)    RREPEAT120(macro, DEC_(data))   macro(data, 120)
#define RREPEAT122(macro, data)    RREPEAT121(macro, DEC_(data))   macro(data, 121)
#define RREPEAT123(macro, data)    RREPEAT122(macro, DEC_(data))   macro(data, 122)
#define RREPEAT124(macro, data)    RREPEAT123(macro, DEC_(data))   macro(data, 123)
#define RREPEAT125(macro, data)    RREPEAT124(macro, DEC_(data))   macro(data, 124)
#define RREPEAT126(macro, data)    RREPEAT125(macro, DEC_(data))   macro(data, 125)
#define RREPEAT127(macro, data)    RREPEAT126(macro, DEC_(data))   macro(data, 126)
#define RREPEAT128(macro, data)    RREPEAT127(macro, DEC_(data))   macro(data, 127)
#define RREPEAT129(macro, data)    RREPEAT128(macro, DEC_(data))   macro(data, 128)
#define RREPEAT130(macro, data)    RREPEAT129(macro, DEC_(data))   macro(data, 129)
#define RREPEAT131(macro, data)    RREPEAT130(macro, DEC_(data))   macro(data, 130)
#define RREPEAT132(macro, data)    RREPEAT131(macro, DEC_(data))   macro(data, 131)
#define RREPEAT133(macro, data)    RREPEAT132(macro, DEC_(data))   macro(data, 132)
#define RREPEAT134(macro, data)    RREPEAT133(macro, DEC_(data))   macro(data, 133)
#define RREPEAT135(macro, data)    RREPEAT134(macro, DEC_(data))   macro(data, 134)
#define RREPEAT136(macro, data)    RREPEAT135(macro, DEC_(data))   macro(data, 135)
#define RREPEAT137(macro, data)    RREPEAT136(macro, DEC_(data))   macro(data, 136)
#define RREPEAT138(macro, data)    RREPEAT137(macro, DEC_(data))   macro(data, 137)
#define RREPEAT139(macro, data)    RREPEAT138(macro, DEC_(data))   macro(data, 138)
#define RREPEAT140(macro, data)    RREPEAT139(macro, DEC_(data))   macro(data, 139)
#define RREPEAT141(macro, data)    RREPEAT140(macro, DEC_(data))   macro(data, 140)
#define RREPEAT142(macro, data)    RREPEAT141(macro, DEC_(data))   macro(data, 141)
#define RREPEAT143(macro, data)    RREPEAT142(macro, DEC_(data))   macro(data, 142)
#define RREPEAT144(macro, data)    RREPEAT143(macro, DEC_(data))   macro(data, 143)
#define RREPEAT145(macro, data)    RREPEAT144(macro, DEC_(data))   macro(data, 144)
#define RREPEAT146(macro, data)    RREPEAT145(macro, DEC_(data))   macro(data, 145)
#define RREPEAT147(macro, data)    RREPEAT146(macro, DEC_(data))   macro(data, 146)
#define RREPEAT148(macro, data)    RREPEAT147(macro, DEC_(data))   macro(data, 147)
#define RREPEAT149(macro, data)    RREPEAT148(macro, DEC_(data))   macro(data, 148)
#define RREPEAT150(macro, data)    RREPEAT149(macro, DEC_(data))   macro(data, 149)
#define RREPEAT151(macro, data)    RREPEAT150(macro, DEC_(data))   macro(data, 150)
#define RREPEAT152(macro, data)    RREPEAT151(macro, DEC_(data))   macro(data, 151)
#define RREPEAT153(macro, data)    RREPEAT152(macro, DEC_(data))   macro(data, 152)
#define RREPEAT154(macro, data)    RREPEAT153(macro, DEC_(data))   macro(data, 153)
#define RREPEAT155(macro, data)    RREPEAT154(macro, DEC_(data))   macro(data, 154)
#define RREPEAT156(macro, data)    RREPEAT155(macro, DEC_(data))   macro(data, 155)
#define RREPEAT157(macro, data)    RREPEAT156(macro, DEC_(data))   macro(data, 156)
#define RREPEAT158(macro, data)    RREPEAT157(macro, DEC_(data))   macro(data, 157)
#define RREPEAT159(macro, data)    RREPEAT158(macro, DEC_(data))   macro(data, 158)
#define RREPEAT160(macro, data)    RREPEAT159(macro, DEC_(data))   macro(data, 159)
#define RREPEAT161(macro, data)    RREPEAT160(macro, DEC_(data))   macro(data, 160)
#define RREPEAT162(macro, data)    RREPEAT161(macro, DEC_(data))   macro(data, 161)
#define RREPEAT163(macro, data)    RREPEAT162(macro, DEC_(data))   macro(data, 162)
#define RREPEAT164(macro, data)    RREPEAT163(macro, DEC_(data))   macro(data, 163)
#define RREPEAT165(macro, data)    RREPEAT164(macro, DEC_(data))   macro(data, 164)
#define RREPEAT166(macro, data)    RREPEAT165(macro, DEC_(data))   macro(data, 165)
#define RREPEAT167(macro, data)    RREPEAT166(macro, DEC_(data))   macro(data, 166)
#define RREPEAT168(macro, data)    RREPEAT167(macro, DEC_(data))   macro(data, 167)
#define RREPEAT169(macro, data)    RREPEAT168(macro, DEC_(data))   macro(data, 168)
#define RREPEAT170(macro, data)    RREPEAT169(macro, DEC_(data))   macro(data, 169)
#define RREPEAT171(macro, data)    RREPEAT170(macro, DEC_(data))   macro(data, 170)
#define RREPEAT172(macro, data)    RREPEAT171(macro, DEC_(data))   macro(data, 171)
#define RREPEAT173(macro, data)    RREPEAT172(macro, DEC_(data))   macro(data, 172)
#define RREPEAT174(macro, data)    RREPEAT173(macro, DEC_(data))   macro(data, 173)
#define RREPEAT175(macro, data)    RREPEAT174(macro, DEC_(data))   macro(data, 174)
#define RREPEAT176(macro, data)    RREPEAT175(macro, DEC_(data))   macro(data, 175)
#define RREPEAT177(macro, data)    RREPEAT176(macro, DEC_(data))   macro(data, 176)
#define RREPEAT178(macro, data)    RREPEAT177(macro, DEC_(data))   macro(data, 177)
#define RREPEAT179(macro, data)    RREPEAT178(macro, DEC_(data))   macro(data, 178)
#define RREPEAT180(macro, data)    RREPEAT179(macro, DEC_(data))   macro(data, 179)
#define RREPEAT181(macro, data)    RREPEAT180(macro, DEC_(data))   macro(data, 180)
#define RREPEAT182(macro, data)    RREPEAT181(macro, DEC_(data))   macro(data, 181)
#define RREPEAT183(macro, data)    RREPEAT182(macro, DEC_(data))   macro(data, 182)
#define RREPEAT184(macro, data)    RREPEAT183(macro, DEC_(data))   macro(data, 183)
#define RREPEAT185(macro, data)    RREPEAT184(macro, DEC_(data))   macro(data, 184)
#define RREPEAT186(macro, data)    RREPEAT185(macro, DEC_(data))   macro(data, 185)
#define RREPEAT187(macro, data)    RREPEAT186(macro, DEC_(data))   macro(data, 186)
#define RREPEAT188(macro, data)    RREPEAT187(macro, DEC_(data))   macro(data, 187)
#define RREPEAT189(macro, data)    RREPEAT188(macro, DEC_(data))   macro(data, 188)
#define RREPEAT190(macro, data)    RREPEAT189(macro, DEC_(data))   macro(data, 189)
#define RREPEAT191(macro, data)    RREPEAT190(macro, DEC_(data))   macro(data, 190)
#define RREPEAT192(macro, data)    RREPEAT191(macro, DEC_(data))   macro(data, 191)
#define RREPEAT193(macro, data)    RREPEAT192(macro, DEC_(data))   macro(data, 192)
#define RREPEAT194(macro, data)    RREPEAT193(macro, DEC_(data))   macro(data, 193)
#define RREPEAT195(macro, data)    RREPEAT194(macro, DEC_(data))   macro(data, 194)
#define RREPEAT196(macro, data)    RREPEAT195(macro, DEC_(data))   macro(data, 195)
#define RREPEAT197(macro, data)    RREPEAT196(macro, DEC_(data))   macro(data, 196)
#define RREPEAT198(macro, data)    RREPEAT197(macro, DEC_(data))   macro(data, 197)
#define RREPEAT199(macro, data)    RREPEAT198(macro, DEC_(data))   macro(data, 198)
#define RREPEAT200(macro, data)    RREPEAT199(macro, DEC_(data))   macro(data, 199)
#define RREPEAT201(macro, data)    RREPEAT200(macro, DEC_(data))   macro(data, 200)
#define RREPEAT202(macro, data)    RREPEAT201(macro, DEC_(data))   macro(data, 201)
#define RREPEAT203(macro, data)    RREPEAT202(macro, DEC_(data))   macro(data, 202)
#define RREPEAT204(macro, data)    RREPEAT203(macro, DEC_(data))   macro(data, 203)
#define RREPEAT205(macro, data)    RREPEAT204(macro, DEC_(data))   macro(data, 204)
#define RREPEAT206(macro, data)    RREPEAT205(macro, DEC_(data))   macro(data, 205)
#define RREPEAT207(macro, data)    RREPEAT206(macro, DEC_(data))   macro(data, 206)
#define RREPEAT208(macro, data)    RREPEAT207(macro, DEC_(data))   macro(data, 207)
#define RREPEAT209(macro, data)    RREPEAT208(macro, DEC_(data))   macro(data, 208)
#define RREPEAT210(macro, data)    RREPEAT209(macro, DEC_(data))   macro(data, 209)
#define RREPEAT211(macro, data)    RREPEAT210(macro, DEC_(data))   macro(data, 210)
#define RREPEAT212(macro, data)    RREPEAT211(macro, DEC_(data))   macro(data, 211)
#define RREPEAT213(macro, data)    RREPEAT212(macro, DEC_(data))   macro(data, 212)
#define RREPEAT214(macro, data)    RREPEAT213(macro, DEC_(data))   macro(data, 213)
#define RREPEAT215(macro, data)    RREPEAT214(macro, DEC_(data))   macro(data, 214)
#define RREPEAT216(macro, data)    RREPEAT215(macro, DEC_(data))   macro(data, 215)
#define RREPEAT217(macro, data)    RREPEAT216(macro, DEC_(data))   macro(data, 216)
#define RREPEAT218(macro, data)    RREPEAT217(macro, DEC_(data))   macro(data, 217)
#define RREPEAT219(macro, data)    RREPEAT218(macro, DEC_(data))   macro(data, 218)
#define RREPEAT220(macro, data)    RREPEAT219(macro, DEC_(data))   macro(data, 219)
#define RREPEAT221(macro, data)    RREPEAT220(macro, DEC_(data))   macro(data, 220)
#define RREPEAT222(macro, data)    RREPEAT221(macro, DEC_(data))   macro(data, 221)
#define RREPEAT223(macro, data)    RREPEAT222(macro, DEC_(data))   macro(data, 222)
#define RREPEAT224(macro, data)    RREPEAT223(macro, DEC_(data))   macro(data, 223)
#define RREPEAT225(macro, data)    RREPEAT224(macro, DEC_(data))   macro(data, 224)
#define RREPEAT226(macro, data)    RREPEAT225(macro, DEC_(data))   macro(data, 225)
#define RREPEAT227(macro, data)    RREPEAT226(macro, DEC_(data))   macro(data, 226)
#define RREPEAT228(macro, data)    RREPEAT227(macro, DEC_(data))   macro(data, 227)
#define RREPEAT229(macro, data)    RREPEAT228(macro, DEC_(data))   macro(data, 228)
#define RREPEAT230(macro, data)    RREPEAT229(macro, DEC_(data))   macro(data, 229)
#define RREPEAT231(macro, data)    RREPEAT230(macro, DEC_(data))   macro(data, 230)
#define RREPEAT232(macro, data)    RREPEAT231(macro, DEC_(data))   macro(data, 231)
#define RREPEAT233(macro, data)    RREPEAT232(macro, DEC_(data))   macro(data, 232)
#define RREPEAT234(macro, data)    RREPEAT233(macro, DEC_(data))   macro(data, 233)
#define RREPEAT235(macro, data)    RREPEAT234(macro, DEC_(data))   macro(data, 234)
#define RREPEAT236(macro, data)    RREPEAT235(macro, DEC_(data))   macro(data, 235)
#define RREPEAT237(macro, data)    RREPEAT236(macro, DEC_(data))   macro(data, 236)
#define RREPEAT238(macro, data)    RREPEAT237(macro, DEC_(data))   macro(data, 237)
#define RREPEAT239(macro, data)    RREPEAT238(macro, DEC_(data))   macro(data, 238)
#define RREPEAT240(macro, data)    RREPEAT239(macro, DEC_(data))   macro(data, 239)
#define RREPEAT241(macro, data)    RREPEAT240(macro, DEC_(data))   macro(data, 240)
#define RREPEAT242(macro, data)    RREPEAT241(macro, DEC_(data))   macro(data, 241)
#define RREPEAT243(macro, data)    RREPEAT242(macro, DEC_(data))   macro(data, 242)
#define RREPEAT244(macro, data)    RREPEAT243(macro, DEC_(data))   macro(data, 243)
#define RREPEAT245(macro, data)    RREPEAT244(macro, DEC_(data))   macro(data, 244)
#define RREPEAT246(macro, data)    RREPEAT245(macro, DEC_(data))   macro(data, 245)
#define RREPEAT247(macro, data)    RREPEAT246(macro, DEC_(data))   macro(data, 246)
#define RREPEAT248(macro, data)    RREPEAT247(macro, DEC_(data))   macro(data, 247)
#define RREPEAT249(macro, data)    RREPEAT248(macro, DEC_(data))   macro(data, 248)
#define RREPEAT250(macro, data)    RREPEAT249(macro, DEC_(data))   macro(data, 249)
#define RREPEAT251(macro, data)    RREPEAT250(macro, DEC_(data))   macro(data, 250)
#define RREPEAT252(macro, data)    RREPEAT251(macro, DEC_(data))   macro(data, 251)
#define RREPEAT253(macro, data)    RREPEAT252(macro, DEC_(data))   macro(data, 252)
#define RREPEAT254(macro, data)    RREPEAT253(macro, DEC_(data))   macro(data, 253)
#define RREPEAT255(macro, data)    RREPEAT254(macro, DEC_(data))   macro(data, 254)
#define RREPEAT256(macro, data)    RREPEAT255(macro, DEC_(data))   macro(data, 255)

#define DEC_256  255
#define DEC_255  254
#define DEC_254  253
#define DEC_253  252
#define DEC_252  251
#define DEC_251  250
#define DEC_250  249
#define DEC_249  248
#define DEC_248  247
#define DEC_247  246
#define DEC_246  245
#define DEC_245  244
#define DEC_244  243
#define DEC_243  242
#define DEC_242  241
#define DEC_241  240
#define DEC_240  239
#define DEC_239  238
#define DEC_238  237
#define DEC_237  236
#define DEC_236  235
#define DEC_235  234
#define DEC_234  233
#define DEC_233  232
#define DEC_232  231
#define DEC_231  230
#define DEC_230  229
#define DEC_229  228
#define DEC_228  227
#define DEC_227  226
#define DEC_226  225
#define DEC_225  224
#define DEC_224  223
#define DEC_223  222
#define DEC_222  221
#define DEC_221  220
#define DEC_220  219
#define DEC_219  218
#define DEC_218  217
#define DEC_217  216
#define DEC_216  215
#define DEC_215  214
#define DEC_214  213
#define DEC_213  212
#define DEC_212  211
#define DEC_211  210
#define DEC_210  209
#define DEC_209  208
#define DEC_208  207
#define DEC_207  206
#define DEC_206  205
#define DEC_205  204
#define DEC_204  203
#define DEC_203  202
#define DEC_202  201
#define DEC_201  200
#define DEC_200  199
#define DEC_199  198
#define DEC_198  197
#define DEC_197  196
#define DEC_196  195
#define DEC_195  194
#define DEC_194  193
#define DEC_193  192
#define DEC_192  191
#define DEC_191  190
#define DEC_190  189
#define DEC_189  188
#define DEC_188  187
#define DEC_187  186
#define DEC_186  185
#define DEC_185  184
#define DEC_184  183
#define DEC_183  182
#define DEC_182  181
#define DEC_181  180
#define DEC_180  179
#define DEC_179  178
#define DEC_178  177
#define DEC_177  176
#define DEC_176  175
#define DEC_175  174
#define DEC_174  173
#define DEC_173  172
#define DEC_172  171
#define DEC_171  170
#define DEC_170  169
#define DEC_169  168
#define DEC_168  167
#define DEC_167  166
#define DEC_166  165
#define DEC_165  164
#define DEC_164  163
#define DEC_163  162
#define DEC_162  161
#define DEC_161  160
#define DEC_160  159
#define DEC_159  158
#define DEC_158  157
#define DEC_157  156
#define DEC_156  155
#define DEC_155  154
#define DEC_154  153
#define DEC_153  152
#define DEC_152  151
#define DEC_151  150
#define DEC_150  149
#define DEC_149  148
#define DEC_148  147
#define DEC_147  146
#define DEC_146  145
#define DEC_145  144
#define DEC_144  143
#define DEC_143  142
#define DEC_142  141
#define DEC_141  140
#define DEC_140  139
#define DEC_139  138
#define DEC_138  137
#define DEC_137  136
#define DEC_136  135
#define DEC_135  134
#define DEC_134  133
#define DEC_133  132
#define DEC_132  131
#define DEC_131  130
#define DEC_130  129
#define DEC_129  128
#define DEC_128  127
#define DEC_127  126
#define DEC_126  125
#define DEC_125  124
#define DEC_124  123
#define DEC_123  122
#define DEC_122  121
#define DEC_121  120
#define DEC_120  119
#define DEC_119  118
#define DEC_118  117
#define DEC_117  116
#define DEC_116  115
#define DEC_115  114
#define DEC_114  113
#define DEC_113  112
#define DEC_112  111
#define DEC_111  110
#define DEC_110  109
#define DEC_109  108
#define DEC_108  107
#define DEC_107  106
#define DEC_106  105
#define DEC_105  104
#define DEC_104  103
#define DEC_103  102
#define DEC_102  101
#define DEC_101  100
#define DEC_100  99
#define DEC_99   98
#define DEC_98   97
#define DEC_97   96
#define DEC_96   95
#define DEC_95   94
#define DEC_94   93
#define DEC_93   92
#define DEC_92   91
#define DEC_91   90
#define DEC_90   89
#define DEC_89   88
#define DEC_88   87
#define DEC_87   86
#define DEC_86   85
#define DEC_85   84
#define DEC_84   83
#define DEC_83   82
#define DEC_82   81
#define DEC_81   80
#define DEC_80   79
#define DEC_79   78
#define DEC_78   77
#define DEC_77   76
#define DEC_76   75
#define DEC_75   74
#define DEC_74   73
#define DEC_73   72
#define DEC_72   71
#define DEC_71   70
#define DEC_70   69
#define DEC_69   68
#define DEC_68   67
#define DEC_67   66
#define DEC_66   65
#define DEC_65   64
#define DEC_64   63
#define DEC_63   62
#define DEC_62   61
#define DEC_61   60
#define DEC_60   59
#define DEC_59   58
#define DEC_58   57
#define DEC_57   56
#define DEC_56   55
#define DEC_55   54
#define DEC_54   53
#define DEC_53   52
#define DEC_52   51
#define DEC_51   50
#define DEC_50   49
#define DEC_49   48
#define DEC_48   47
#define DEC_47   46
#define DEC_46   45
#define DEC_45   44
#define DEC_44   43
#define DEC_43   42
#define DEC_42   41
#define DEC_41   40
#define DEC_40   39
#define DEC_39   38
#define DEC_38   37
#define DEC_37   36
#define DEC_36   35
#define DEC_35   34
#define DEC_34   33
#define DEC_33   32
#define DEC_32   31
#define DEC_31   30
#define DEC_30   29
#define DEC_29   28
#define DEC_28   27
#define DEC_27   26
#define DEC_26   25
#define DEC_25   24
#define DEC_24   23
#define DEC_23   22
#define DEC_22   21
#define DEC_21   20
#define DEC_20   19
#define DEC_19   18
#define DEC_18   17
#define DEC_17   16
#define DEC_16   15
#define DEC_15   14
#define DEC_14   13
#define DEC_13   12
#define DEC_12   11
#define DEC_11   10
#define DEC_10   9
#define DEC_9    8
#define DEC_8    7
#define DEC_7    6
#define DEC_6    5
#define DEC_5    4
#define DEC_4    3
#define DEC_3    2
#define DEC_2    1
#define DEC_1    0
#define DEC_(n)  DEC_##n

#endif // __REPEAT_H_
