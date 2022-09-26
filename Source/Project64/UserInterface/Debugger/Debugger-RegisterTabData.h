#pragma once

struct FieldPair
{
    const WORD LabelId;
    const WORD EditId;
};

using FieldPairWithStopCallback = std::function<bool(const CWindow & Label, const CWindow & Edit)>;
using FieldPairCallback = std::function<void(const CWindow & Label, const CWindow & Edit)>;

struct TabRecord
{
    const size_t FieldCount = 0;
    const FieldPair * Fields = nullptr;

    constexpr TabRecord(size_t arraySize, const FieldPair * pairs) :
        FieldCount{arraySize / sizeof(*pairs)},
        Fields{pairs}
    {
    }

    int GetLabelIndex(WORD ctrl) const
    {
        for (int i = 0, end = FieldCount; i < end; i++)
        {
            const FieldPair * pair = (Fields + i);
            if (pair->LabelId == ctrl)
            {
                return i;
            }
        }
        return -1;
    }

    int GetEditIndex(WORD ctrl) const
    {
        for (int i = 0, end = FieldCount; i < end; i++)
        {
            const FieldPair * pair = (Fields + i);
            if (pair->EditId == ctrl)
            {
                return i;
            }
        }
        return -1;
    }

    void Iterate(const CWindow & parent, FieldPairWithStopCallback callback) const
    {
        for (size_t i = 0, end = FieldCount; i < end; i++)
        {
            const FieldPair * pair = (Fields + i);
            if (callback(parent.GetDescendantWindow(pair->LabelId), parent.GetDescendantWindow(pair->EditId)))
            {
                break;
            }
        }
    }

    void Iterate(const CWindow & parent, FieldPairCallback callback) const
    {
        for (size_t i = 0, end = FieldCount; i < end; i++)
        {
            const FieldPair * pair = (Fields + i);
            callback(parent.GetDescendantWindow(pair->LabelId), parent.GetDescendantWindow(pair->EditId));
        }
    }
};

class TabData
{
public:
    static constexpr FieldPair GPRFields[] =
    {
        {IDC_R0_LBL, IDC_R0_EDIT},
        {IDC_R1_LBL, IDC_R1_EDIT},
        {IDC_R2_LBL, IDC_R2_EDIT},
        {IDC_R3_LBL, IDC_R3_EDIT},
        {IDC_R4_LBL, IDC_R4_EDIT},
        {IDC_R5_LBL, IDC_R5_EDIT},
        {IDC_R6_LBL, IDC_R6_EDIT},
        {IDC_R7_LBL, IDC_R7_EDIT},
        {IDC_R8_LBL, IDC_R8_EDIT},
        {IDC_R9_LBL, IDC_R9_EDIT},
        {IDC_R10_LBL, IDC_R10_EDIT},
        {IDC_R11_LBL, IDC_R11_EDIT},
        {IDC_R12_LBL, IDC_R12_EDIT},
        {IDC_R13_LBL, IDC_R13_EDIT},
        {IDC_R14_LBL, IDC_R14_EDIT},
        {IDC_R15_LBL, IDC_R15_EDIT},
        {IDC_R16_LBL, IDC_R16_EDIT},
        {IDC_R17_LBL, IDC_R17_EDIT},
        {IDC_R18_LBL, IDC_R18_EDIT},
        {IDC_R19_LBL, IDC_R19_EDIT},
        {IDC_R20_LBL, IDC_R20_EDIT},
        {IDC_R21_LBL, IDC_R21_EDIT},
        {IDC_R22_LBL, IDC_R22_EDIT},
        {IDC_R23_LBL, IDC_R23_EDIT},
        {IDC_R24_LBL, IDC_R24_EDIT},
        {IDC_R25_LBL, IDC_R25_EDIT},
        {IDC_R26_LBL, IDC_R26_EDIT},
        {IDC_R27_LBL, IDC_R27_EDIT},
        {IDC_R28_LBL, IDC_R28_EDIT},
        {IDC_R29_LBL, IDC_R29_EDIT},
        {IDC_R30_LBL, IDC_R30_EDIT},
        {IDC_R31_LBL, IDC_R31_EDIT},
    };

    static constexpr TabRecord GPR = TabRecord{sizeof(GPRFields), GPRFields};

    static constexpr FieldPair FPRFields[] =
    {
        {IDC_F0_LBL, IDC_F0_EDIT},
        {IDC_F1_LBL, IDC_F1_EDIT},
        {IDC_F2_LBL, IDC_F2_EDIT},
        {IDC_F3_LBL, IDC_F3_EDIT},
        {IDC_F4_LBL, IDC_F4_EDIT},
        {IDC_F5_LBL, IDC_F5_EDIT},
        {IDC_F6_LBL, IDC_F6_EDIT},
        {IDC_F7_LBL, IDC_F7_EDIT},
        {IDC_F8_LBL, IDC_F8_EDIT},
        {IDC_F9_LBL, IDC_F9_EDIT},
        {IDC_F10_LBL, IDC_F10_EDIT},
        {IDC_F11_LBL, IDC_F11_EDIT},
        {IDC_F12_LBL, IDC_F12_EDIT},
        {IDC_F13_LBL, IDC_F13_EDIT},
        {IDC_F14_LBL, IDC_F14_EDIT},
        {IDC_F15_LBL, IDC_F15_EDIT},
        {IDC_F16_LBL, IDC_F16_EDIT},
        {IDC_F17_LBL, IDC_F17_EDIT},
        {IDC_F18_LBL, IDC_F18_EDIT},
        {IDC_F19_LBL, IDC_F19_EDIT},
        {IDC_F20_LBL, IDC_F20_EDIT},
        {IDC_F21_LBL, IDC_F21_EDIT},
        {IDC_F22_LBL, IDC_F22_EDIT},
        {IDC_F23_LBL, IDC_F23_EDIT},
        {IDC_F24_LBL, IDC_F24_EDIT},
        {IDC_F25_LBL, IDC_F25_EDIT},
        {IDC_F26_LBL, IDC_F26_EDIT},
        {IDC_F27_LBL, IDC_F27_EDIT},
        {IDC_F28_LBL, IDC_F28_EDIT},
        {IDC_F29_LBL, IDC_F29_EDIT},
        {IDC_F30_LBL, IDC_F30_EDIT},
        {IDC_F31_LBL, IDC_F31_EDIT},
    };

    static constexpr TabRecord FPR = TabRecord{sizeof(FPRFields), FPRFields};

    static constexpr FieldPair COP0Fields[] =
    {
        {IDC_COP0_0_LBL, IDC_COP0_0_EDIT},
        {IDC_COP0_1_LBL, IDC_COP0_1_EDIT},
        {IDC_COP0_2_LBL, IDC_COP0_2_EDIT},
        {IDC_COP0_3_LBL, IDC_COP0_3_EDIT},
        {IDC_COP0_4_LBL, IDC_COP0_4_EDIT},
        {IDC_COP0_5_LBL, IDC_COP0_5_EDIT},
        {IDC_COP0_6_LBL, IDC_COP0_6_EDIT},
        {IDC_COP0_7_LBL, IDC_COP0_7_EDIT},
        {IDC_COP0_8_LBL, IDC_COP0_8_EDIT},
        {IDC_COP0_9_LBL, IDC_COP0_9_EDIT},
        {IDC_COP0_10_LBL, IDC_COP0_10_EDIT},
        {IDC_COP0_11_LBL, IDC_COP0_11_EDIT},
        {IDC_COP0_12_LBL, IDC_COP0_12_EDIT},
        {IDC_COP0_13_LBL, IDC_COP0_13_EDIT},
        {IDC_COP0_14_LBL, IDC_COP0_14_EDIT},
        {IDC_COP0_15_LBL, IDC_COP0_15_EDIT},
        {IDC_COP0_16_LBL, IDC_COP0_16_EDIT},
        {IDC_COP0_17_LBL, IDC_COP0_17_EDIT},
        {IDC_COP0_18_LBL, IDC_COP0_18_EDIT},
    };

    static constexpr TabRecord COP0 = TabRecord{sizeof(COP0Fields), COP0Fields};

    static constexpr FieldPair RDRAMFields[] =
    {
        {IDC_RDRAM00_LBL, IDC_RDRAM00_EDIT},
        {IDC_RDRAM04_LBL, IDC_RDRAM04_EDIT},
        {IDC_RDRAM08_LBL, IDC_RDRAM08_EDIT},
        {IDC_RDRAM0C_LBL, IDC_RDRAM0C_EDIT},
        {IDC_RDRAM10_LBL, IDC_RDRAM10_EDIT},
        {IDC_RDRAM14_LBL, IDC_RDRAM14_EDIT},
        {IDC_RDRAM18_LBL, IDC_RDRAM18_EDIT},
        {IDC_RDRAM1C_LBL, IDC_RDRAM1C_EDIT},
        {IDC_RDRAM20_LBL, IDC_RDRAM20_EDIT},
        {IDC_RDRAM24_LBL, IDC_RDRAM24_EDIT},
    };

    static constexpr TabRecord RDRAM = TabRecord{sizeof(RDRAMFields), RDRAMFields};

    static constexpr FieldPair SPFields[] =
    {
        {IDC_SP00_LBL, IDC_SP00_EDIT},
        {IDC_SP04_LBL, IDC_SP04_EDIT},
        {IDC_SP08_LBL, IDC_SP08_EDIT},
        {IDC_SP0C_LBL, IDC_SP0C_EDIT},
        {IDC_SP10_LBL, IDC_SP10_EDIT},
        {IDC_SP14_LBL, IDC_SP14_EDIT},
        {IDC_SP18_LBL, IDC_SP18_EDIT},
        {IDC_SP1C_LBL, IDC_SP1C_EDIT},
    };

    static constexpr TabRecord SP = TabRecord{sizeof(SPFields), SPFields};

    static constexpr FieldPair DPCFields[] =
    {
        {IDC_DPC00_LBL, IDC_DPC00_EDIT},
        {IDC_DPC04_LBL, IDC_DPC04_EDIT},
        {IDC_DPC08_LBL, IDC_DPC08_EDIT},
        {IDC_DPC0C_LBL, IDC_DPC0C_EDIT},
        {IDC_DPC10_LBL, IDC_DPC10_EDIT},
        {IDC_DPC14_LBL, IDC_DPC14_EDIT},
        {IDC_DPC18_LBL, IDC_DPC18_EDIT},
        {IDC_DPC1C_LBL, IDC_DPC1C_EDIT},
    };

    static constexpr TabRecord DPC = TabRecord{sizeof(DPCFields), DPCFields};

    static constexpr FieldPair MIFields[] =
    {
        {IDC_MI00_LBL, IDC_MI00_EDIT},
        {IDC_MI04_LBL, IDC_MI04_EDIT},
        {IDC_MI08_LBL, IDC_MI08_EDIT},
        {IDC_MI0C_LBL, IDC_MI0C_EDIT},
    };

    static constexpr TabRecord MI = TabRecord{sizeof(MIFields), MIFields};

    static constexpr FieldPair VIFields[] =
    {
        {IDC_VI00_LBL, IDC_VI00_EDIT},
        {IDC_VI04_LBL, IDC_VI04_EDIT},
        {IDC_VI08_LBL, IDC_VI08_EDIT},
        {IDC_VI0C_LBL, IDC_VI0C_EDIT},
        {IDC_VI10_LBL, IDC_VI10_EDIT},
        {IDC_VI14_LBL, IDC_VI14_EDIT},
        {IDC_VI18_LBL, IDC_VI18_EDIT},
        {IDC_VI1C_LBL, IDC_VI1C_EDIT},
        {IDC_VI20_LBL, IDC_VI20_EDIT},
        {IDC_VI24_LBL, IDC_VI24_EDIT},
        {IDC_VI28_LBL, IDC_VI28_EDIT},
        {IDC_VI2C_LBL, IDC_VI2C_EDIT},
        {IDC_VI30_LBL, IDC_VI30_EDIT},
        {IDC_VI34_LBL, IDC_VI34_EDIT},
    };

    static constexpr TabRecord VI = TabRecord{sizeof(VIFields), VIFields};

    static constexpr FieldPair AIFields[] =
    {
        {IDC_AI00_LBL, IDC_AI00_EDIT},
        {IDC_AI04_LBL, IDC_AI04_EDIT},
        {IDC_AI08_LBL, IDC_AI08_EDIT},
        {IDC_AI0C_LBL, IDC_AI0C_EDIT},
        {IDC_AI10_LBL, IDC_AI10_EDIT},
        {IDC_AI14_LBL, IDC_AI14_EDIT},
    };

    static constexpr TabRecord AI = TabRecord{sizeof(AIFields), AIFields};

    static constexpr FieldPair PIFields[] =
    {
        {IDC_PI00_LBL, IDC_PI00_EDIT},
        {IDC_PI04_LBL, IDC_PI04_EDIT},
        {IDC_PI08_LBL, IDC_PI08_EDIT},
        {IDC_PI0C_LBL, IDC_PI0C_EDIT},
        {IDC_PI10_LBL, IDC_PI10_EDIT},
        {IDC_PI14_LBL, IDC_PI14_EDIT},
        {IDC_PI18_LBL, IDC_PI18_EDIT},
        {IDC_PI1C_LBL, IDC_PI1C_EDIT},
        {IDC_PI20_LBL, IDC_PI20_EDIT},
        {IDC_PI24_LBL, IDC_PI24_EDIT},
        {IDC_PI28_LBL, IDC_PI28_EDIT},
        {IDC_PI2C_LBL, IDC_PI2C_EDIT},
        {IDC_PI30_LBL, IDC_PI30_EDIT},
    };

    static constexpr TabRecord PI = TabRecord{sizeof(PIFields), PIFields};

    static constexpr FieldPair RIFields[] =
    {
        {IDC_RI00_LBL, IDC_RI00_EDIT},
        {IDC_RI04_LBL, IDC_RI04_EDIT},
        {IDC_RI08_LBL, IDC_RI08_EDIT},
        {IDC_RI0C_LBL, IDC_RI0C_EDIT},
        {IDC_RI10_LBL, IDC_RI10_EDIT},
        {IDC_RI14_LBL, IDC_RI14_EDIT},
        {IDC_RI18_LBL, IDC_RI18_EDIT},
        {IDC_RI1C_LBL, IDC_RI1C_EDIT},
    };

    static constexpr TabRecord RI = TabRecord{sizeof(RIFields), RIFields};

    static constexpr FieldPair SIFields[] =
    {
        {IDC_SI00_LBL, IDC_SI00_EDIT},
        {IDC_SI04_LBL, IDC_SI04_EDIT},
        {IDC_SI08_LBL, IDC_SI08_EDIT},
        {IDC_SI0C_LBL, IDC_SI0C_EDIT},
    };

    static constexpr TabRecord SI = TabRecord{sizeof(SIFields), SIFields};

    static constexpr FieldPair DDFields[] =
    {
        {IDC_DD00_LBL, IDC_DD00_EDIT},
        {IDC_DD04_LBL, IDC_DD04_EDIT},
        {IDC_DD08_LBL, IDC_DD08_EDIT},
        {IDC_DD0C_LBL, IDC_DD0C_EDIT},
        {IDC_DD10_LBL, IDC_DD10_EDIT},
        {IDC_DD14_LBL, IDC_DD14_EDIT},
        {IDC_DD18_LBL, IDC_DD18_EDIT},
        {IDC_DD1C_LBL, IDC_DD1C_EDIT},
        {IDC_DD20_LBL, IDC_DD20_EDIT},
        {IDC_DD24_LBL, IDC_DD24_EDIT},
        {IDC_DD28_LBL, IDC_DD28_EDIT},
        {IDC_DD2C_LBL, IDC_DD2C_EDIT},
        {IDC_DD30_LBL, IDC_DD30_EDIT},
        {IDC_DD34_LBL, IDC_DD34_EDIT},
        {IDC_DD38_LBL, IDC_DD38_EDIT},
        {IDC_DD3C_LBL, IDC_DD3C_EDIT},
        {IDC_DD40_LBL, IDC_DD40_EDIT},
        {IDC_DD44_LBL, IDC_DD44_EDIT},
        {IDC_DD48_LBL, IDC_DD48_EDIT},
    };

    static constexpr TabRecord DD = TabRecord{sizeof(DDFields), DDFields};
};
