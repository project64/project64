#include "stdafx.h"

CDMA::CDMA(CFlashram & FlashRam, CSram & Sram) :
	m_FlashRam(FlashRam),
	m_Sram(Sram)
{
}

void CDMA::OnFirstDMA (void) {
	switch (_Rom->CicChipID()) {
	case 1: *(DWORD *)&((_MMU->Rdram())[0x318]) = _MMU->RdramSize(); break;
	case 2: *(DWORD *)&((_MMU->Rdram())[0x318]) = _MMU->RdramSize(); break;
	case 3: *(DWORD *)&((_MMU->Rdram())[0x318]) = _MMU->RdramSize(); break;
	case 5: *(DWORD *)&((_MMU->Rdram())[0x3F0]) = _MMU->RdramSize(); break;
	case 6: *(DWORD *)&((_MMU->Rdram())[0x318]) = _MMU->RdramSize(); break;
	default: _Notify->DisplayError("Unhandled CicChip(%d) in first DMA",_Rom->CicChipID());
	}
}

void CDMA::PI_DMA_READ (void) {
//	PI_STATUS_REG |= PI_STATUS_DMA_BUSY;

	if ( _Reg->PI_DRAM_ADDR_REG + _Reg->PI_RD_LEN_REG + 1 > _MMU->RdramSize()) {
#ifndef EXTERNAL_RELEASE
		_Notify->DisplayError("PI_DMA_READ not in Memory");
#endif
		_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
		_Reg->MI_INTR_REG |= MI_INTR_PI;
		_Reg->CheckInterrupts();
		return;
	}

	if ( _Reg->PI_CART_ADDR_REG >= 0x08000000 && _Reg->PI_CART_ADDR_REG <= 0x08010000) {
		if (_System->m_SaveUsing == SaveChip_Auto) { _System->m_SaveUsing = SaveChip_Sram; }
		if (_System->m_SaveUsing == SaveChip_Sram) {
			m_Sram.DmaToSram(
				_MMU->Rdram() + _Reg->PI_DRAM_ADDR_REG,
				_Reg->PI_CART_ADDR_REG - 0x08000000,
				_Reg->PI_RD_LEN_REG + 1
			);
			_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
			_Reg->MI_INTR_REG |= MI_INTR_PI;
			_Reg->CheckInterrupts();
			return;
		}
		if (_System->m_SaveUsing == SaveChip_FlashRam) {
			m_FlashRam.DmaToFlashram(
				_MMU->Rdram()+_Reg->PI_DRAM_ADDR_REG,
				_Reg->PI_CART_ADDR_REG - 0x08000000,
				_Reg->PI_WR_LEN_REG + 1
			);
			_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
			_Reg->MI_INTR_REG |= MI_INTR_PI;
			_Reg->CheckInterrupts();
			return;
		}
	}
	if (_System->m_SaveUsing == SaveChip_FlashRam) 
	{
		_Notify->DisplayError("**** FLashRam DMA Read address %X *****",_Reg->PI_CART_ADDR_REG);
		_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
		_Reg->MI_INTR_REG |= MI_INTR_PI;
		_Reg->CheckInterrupts();
		return;
	}
#ifndef EXTERNAL_RELEASE
	_Notify->DisplayError("PI_DMA_READ where are you dmaing to ?");
#endif	
	_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
	_Reg->MI_INTR_REG |= MI_INTR_PI;
	_Reg->CheckInterrupts();
	return;
}

void CDMA::PI_DMA_WRITE (void) {

	_Reg->PI_STATUS_REG |= PI_STATUS_DMA_BUSY;
	if ( _Reg->PI_DRAM_ADDR_REG + _Reg->PI_WR_LEN_REG + 1 > _MMU->RdramSize()) 
	{
		if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("PI_DMA_WRITE not in Memory"); }
		_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
		_Reg->MI_INTR_REG |= MI_INTR_PI;
		_Reg->CheckInterrupts();
		return;
	}

	if ( _Reg->PI_CART_ADDR_REG >= 0x08000000 && _Reg->PI_CART_ADDR_REG <= 0x08010000) {
		if (_System->m_SaveUsing == SaveChip_Auto) { _System->m_SaveUsing = SaveChip_Sram; }
		if (_System->m_SaveUsing == SaveChip_Sram) {
			m_Sram.DmaFromSram(
				_MMU->Rdram()+_Reg->PI_DRAM_ADDR_REG,
				_Reg->PI_CART_ADDR_REG - 0x08000000,
				_Reg->PI_WR_LEN_REG + 1
			);
			_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
			_Reg->MI_INTR_REG |= MI_INTR_PI;
			_Reg->CheckInterrupts();
			return;
		}
		if (_System->m_SaveUsing == SaveChip_FlashRam) {
			m_FlashRam.DmaFromFlashram(
				_MMU->Rdram()+_Reg->PI_DRAM_ADDR_REG,
				_Reg->PI_CART_ADDR_REG - 0x08000000,
				_Reg->PI_WR_LEN_REG + 1
			);
			_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
			_Reg->MI_INTR_REG |= MI_INTR_PI;
			_Reg->CheckInterrupts();
		}
		return;
	}

	if ( _Reg->PI_CART_ADDR_REG >= 0x10000000 && _Reg->PI_CART_ADDR_REG <= 0x1FBFFFFF) 
	{
	DWORD i;	
#ifdef tofix
#ifdef ROM_IN_MAPSPACE
		if (WrittenToRom) { 
			DWORD OldProtect;
			VirtualProtect(ROM,m_RomFileSize,PAGE_READONLY, &OldProtect);
		}
#endif
#endif
		BYTE * ROM   = _Rom->GetRomAddress();
		BYTE * RDRAM = _MMU->Rdram();
		_Reg->PI_CART_ADDR_REG -= 0x10000000;
		if (_Reg->PI_CART_ADDR_REG + _Reg->PI_WR_LEN_REG + 1 < _Rom->GetRomSize()) {
			for (i = 0; i < _Reg->PI_WR_LEN_REG + 1; i ++) {
				*(RDRAM+((_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) =  *(ROM+((_Reg->PI_CART_ADDR_REG + i) ^ 3));
			}
		} else {
			DWORD Len;
			Len = _Rom->GetRomSize() - _Reg->PI_CART_ADDR_REG;
			for (i = 0; i < Len; i ++) {
				*(RDRAM+((_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) =  *(ROM+((_Reg->PI_CART_ADDR_REG + i) ^ 3));
			}
			for (i = Len; i < _Reg->PI_WR_LEN_REG + 1 - Len; i ++) {
				*(RDRAM+((_Reg->PI_DRAM_ADDR_REG + i) ^ 3)) =  0;
			}
		}
		_Reg->PI_CART_ADDR_REG += 0x10000000;

		if (!_System->DmaUsed())
		{
			_System->SetDmaUsed(true);
			OnFirstDMA(); 
		}
		if (_Recompiler && _Recompiler->bSMM_PIDMA())
		{
			_Recompiler->ClearRecompCode_Phys(_Reg->PI_DRAM_ADDR_REG, _Reg->PI_WR_LEN_REG,CRecompiler::Remove_DMA);
		}
		_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
		_Reg->MI_INTR_REG |= MI_INTR_PI;
		_Reg->CheckInterrupts();
		//ChangeTimer(PiTimer,(int)(PI_WR_LEN_REG * 8.9) + 50);
		//ChangeTimer(PiTimer,(int)(PI_WR_LEN_REG * 8.9));
		return;
	}
	
	if (_Settings->LoadBool(Debugger_ShowUnhandledMemory)) { _Notify->DisplayError("PI_DMA_WRITE not in ROM"); }
	_Reg->PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
	_Reg->MI_INTR_REG |= MI_INTR_PI;
	_Reg->CheckInterrupts();

}

void CDMA::SP_DMA_READ (void) { 
	_Reg->SP_DRAM_ADDR_REG &= 0x1FFFFFFF;

	if (_Reg->SP_DRAM_ADDR_REG > _MMU->RdramSize()) {
#ifndef EXTERNAL_RELEASE
		_Notify->DisplayError("SP DMA\nSP_DRAM_ADDR_REG not in RDRam space");
#endif
		_Reg->SP_DMA_BUSY_REG = 0;
		_Reg->SP_STATUS_REG  &= ~SP_STATUS_DMA_BUSY;
		return;
	}
	
	if (_Reg->SP_RD_LEN_REG + 1  + (_Reg->SP_MEM_ADDR_REG & 0xFFF) > 0x1000) {
#ifndef EXTERNAL_RELEASE
		_Notify->DisplayError("SP DMA\ncould not fit copy in memory segement");
#endif
		return;		
	}
	
	if ((_Reg->SP_MEM_ADDR_REG & 3) != 0) { _Notify->BreakPoint(__FILE__,__LINE__);  }
	if ((_Reg->SP_DRAM_ADDR_REG & 3) != 0) { _Notify->BreakPoint(__FILE__,__LINE__);  }
	if (((_Reg->SP_RD_LEN_REG + 1) & 3) != 0) { _Notify->BreakPoint(__FILE__,__LINE__);  }

	memcpy( _MMU->Dmem() + (_Reg->SP_MEM_ADDR_REG & 0x1FFF), _MMU->Rdram() + _Reg->SP_DRAM_ADDR_REG,
		_Reg->SP_RD_LEN_REG + 1 );
		
	_Reg->SP_DMA_BUSY_REG = 0;
	_Reg->SP_STATUS_REG  &= ~SP_STATUS_DMA_BUSY;
}

void CDMA::SP_DMA_WRITE (void) { 
	if (_Reg->SP_DRAM_ADDR_REG > _MMU->RdramSize()) {
#ifndef EXTERNAL_RELEASE
		_Notify->DisplayError("SP DMA WRITE\nSP_DRAM_ADDR_REG not in RDRam space");
#endif
		return;
	}
	
	if (_Reg->SP_WR_LEN_REG + 1 + (_Reg->SP_MEM_ADDR_REG & 0xFFF) > 0x1000) {
#ifndef EXTERNAL_RELEASE
		_Notify->DisplayError("SP DMA WRITE\ncould not fit copy in memory segement");
#endif
		return;		
	}

	if ((_Reg->SP_MEM_ADDR_REG & 3) != 0) { _Notify->BreakPoint(__FILE__,__LINE__); }
	if ((_Reg->SP_DRAM_ADDR_REG & 3) != 0) { _Notify->BreakPoint(__FILE__,__LINE__);  }
	if (((_Reg->SP_WR_LEN_REG + 1) & 3) != 0) { _Notify->BreakPoint(__FILE__,__LINE__);  }

	memcpy( _MMU->Rdram() + _Reg->SP_DRAM_ADDR_REG, _MMU->Dmem() + (_Reg->SP_MEM_ADDR_REG & 0x1FFF),
		_Reg->SP_WR_LEN_REG + 1);
		
	_Reg->SP_DMA_BUSY_REG = 0;
	_Reg->SP_STATUS_REG  &= ~SP_STATUS_DMA_BUSY;
}

