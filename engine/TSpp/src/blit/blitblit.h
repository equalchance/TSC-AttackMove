/*******************************************************************************
/*                     O P E N  S O U R C E  --  T S + +                      **
/*******************************************************************************
 *  @brief  Templated blitters.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Copyright (c) 2020-2026 Vinifera contributors
 ******************************************************************************/
#pragma once

#include "always.h"
#include "blitter.h"
#include "rleblitter.h"
#include "alphalighting.h"


typedef enum BlitterType {

    /**
     *  Standard blitters.
     */
    BLITTER_PLAINXLATPTR = 0,
    BLITTER_TRANSXLATPTR,
    BLITTER_TRANSDARKENPTR,
    BLITTER_TRANSZREMAPXLATPTR,
    BLITTER_TRANSLUCENT75PTR,
    BLITTER_TRANSLUCENT50PTR,
    BLITTER_TRANSLUCENT25PTR,
    BLITTER_PLAINXLATZREADPTR,
    BLITTER_TRANSXLATZREADPTR,
    BLITTER_TRANSDARKENZREADPTR,
    BLITTER_TRANSZREMAPXLATZREADPTR,
    BLITTER_TRANSLUCENT75ZREADPTR,
    BLITTER_TRANSLUCENT50ZREADPTR,
    BLITTER_TRANSLUCENT25ZREADPTR,
    BLITTER_TRANSLUCENT75ZREADWARPPTR,
    BLITTER_TRANSLUCENT50ZREADWARPPTR,
    BLITTER_TRANSLUCENT25ZREADWARPPTR,
    BLITTER_PLAINXLATZREADWRITEPTR,
    BLITTER_TRANSXLATZREADWRITEPTR,
    BLITTER_TRANSDARKENZREADWRITEPTR,
    BLITTER_TRANSZREMAPXLATZREADWRITEPTR,
    BLITTER_TRANSLUCENT75ZREADWRITEPTR,
    BLITTER_TRANSLUCENT50ZREADWRITEPTR,
    BLITTER_TRANSLUCENT25ZREADWRITEPTR,
    BLITTER_PLAINXLATALPHAPTR,
    BLITTER_TRANSXLATALPHAPTR,
    BLITTER_TRANSZREMAPXLATALPHAPTR,
    BLITTER_TRANSLUCENT75ALPHAPTR,
    BLITTER_TRANSLUCENT50ALPHAPTR,
    BLITTER_TRANSLUCENT25ALPHAPTR,
    BLITTER_TRANSXLATWRITEALPHAPTR,
    BLITTER_TRANSXLATMULTWRITEALPHAPTR,
    BLITTER_TRANSLUCENTWRITEALPHAPTR,
    BLITTER_TRANSLUCENT50NONZEROALPHAPTR,
    BLITTER_TRANSLUCENT50ZEROALPHAPTR,
    BLITTER_TRANSLUCENT75NONZEROALPHAPTR,
    BLITTER_TRANSLUCENT75ZEROALPHAPTR,
    BLITTER_PLAINXLATALPHA_2PTR,
    BLITTER_TRANSXLATALPHAZREADPTR,
    BLITTER_TRANSZREMAPXLATALPHAZREADPTR,
    BLITTER_TRANSLUCENT75ALPHAZREADPTR,
    BLITTER_TRANSLUCENT50ALPHAZREADPTR,
    BLITTER_TRANSLUCENT25ALPHAZREADPTR,
    BLITTER_TRANSLUCENT75ALPHAZREADWARPPTR,
    BLITTER_TRANSLUCENT50ALPHAZREADWARPPTR,
    BLITTER_TRANSLUCENT25ALPHAZREADWARPPTR,
    BLITTER_PLAINXLATALPHA_3PTR,
    BLITTER_TRANSXLATALPHAZREADWRITEPTR,
    BLITTER_TRANSZREMAPXLATALPHAZREADWRITEPTR,
    BLITTER_TRANSLUCENT75ALPHAZREADWRITEPTR,
    BLITTER_TRANSLUCENT50ALPHAZREADWRITEPTR,
    BLITTER_TRANSLUCENT25ALPHAZREADWRITEPTR,

    BLITTER_COUNT,


    /**
     *  Run-length encoded blitters.
     */
    RLEBLITTER_TRANSXLATPTR = 0,
    RLEBLITTER_TRANSDARKENPTR,
    RLEBLITTER_TRANSZREMAPXLATPTR,
    RLEBLITTER_TRANSLUCENT75PTR,
    RLEBLITTER_TRANSLUCENT50PTR,
    RLEBLITTER_TRANSLUCENT25PTR,
    RLEBLITTER_TRANSXLATZREADPTR,
    RLEBLITTER_TRANSZREMAPXLATZREADPTR,
    RLEBLITTER_TRANSDARKENZREADPTR,
    RLEBLITTER_TRANSLUCENT75ZREADPTR,
    RLEBLITTER_TRANSLUCENT50ZREADPTR,
    RLEBLITTER_TRANSLUCENT25ZREADPTR,
    RLEBLITTER_TRANSLUCENT75ZREADWARPPTR,
    RLEBLITTER_TRANSLUCENT50ZREADWARPPTR,
    RLEBLITTER_TRANSLUCENT25ZREADWARPPTR,
    RLEBLITTER_TRANSXLATZREADWRITEPTR,
    RLEBLITTER_TRANSZREMAPXLATZREADWRITEPTR,
    RLEBLITTER_TRANSDARKENZREADWRITEPTR,
    RLEBLITTER_TRANSLUCENT75ZREADWRITEPTR,
    RLEBLITTER_TRANSLUCENT50ZREADWRITEPTR,
    RLEBLITTER_TRANSLUCENT25ZREADWRITEPTR,
    RLEBLITTER_TRANSXLATALPHAPTR,
    RLEBLITTER_TRANSZREMAPXLATALPHAPTR,
    RLEBLITTER_TRANSLUCENT75ALPHAPTR,
    RLEBLITTER_TRANSLUCENT50ALPHAPTR,
    RLEBLITTER_TRANSLUCENT25ALPHAPTR,
    RLEBLITTER_TRANSXLATALPHAZREADPTR,
    RLEBLITTER_TRANSZREMAPXLATALPHAZREADPTR,
    RLEBLITTER_TRANSLUCENT75ALPHAZREADPTR,
    RLEBLITTER_TRANSLUCENT50ALPHAZREADPTR,
    RLEBLITTER_TRANSLUCENT25ALPHAZREADPTR,
    RLEBLITTER_TRANSLUCENT75ALPHAZREADWARPPTR,
    RLEBLITTER_TRANSLUCENT50ALPHAZREADWARPPTR,
    RLEBLITTER_TRANSLUCENT25ALPHAZREADWARPPTR,
    RLEBLITTER_TRANSXLATALPHAZREADWRITEPTR,
    RLEBLITTER_TRANSZREMAPXLATALPHAZREADWRITEPTR,
    RLEBLITTER_TRANSLUCENT75ALPHAZREADWRITEPTR,
    RLEBLITTER_TRANSLUCENT50ALPHAZREADWRITEPTR,
    RLEBLITTER_TRANSLUCENT25ALPHAZREADWRITEPTR,

    RLEBLITTER_COUNT

} BlitterType;
DEFINE_ENUMERATION_OPERATORS(BlitterType);


template<class T>
class BlitPlain : public Blitter {
	public:
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;
};


/*
**	Blits with transparency checking when and source and dest are same pixel format.
**	This process is not often used.
*/
template<class T>
class BlitTrans : public Blitter {
	public:
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;
};


/*
**	Blits when source 8 bits and dest is T. This process is typically used
**	when loading screen bitmaps or perform other non-transparent image blitting.
**	It is used fairly frequently and is a good candidate for optimization.
*/
template<class T>
class BlitPlainXlat : public Blitter {
	public:
		BlitPlainXlat(T const * translator) : TranslateTable(translator) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
};


template<class T>
class BlitPlainXlatAlpha : public Blitter {
	public:
		BlitPlainXlatAlpha(T const * translator, int arg) : TranslateTable(translator), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~BlitPlainXlatAlpha(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
};


template<class T>
class BlitPlainXlatZRead : public Blitter {
	public:
		BlitPlainXlatZRead(T const * translator) : TranslateTable(translator) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
};


template<class T>
class BlitPlainXlatZReadWrite : public Blitter {
	public:
		BlitPlainXlatZReadWrite(T const * translator) : TranslateTable(translator) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
};


/*
**	Blits with source 8 bit with transparency and dest is T. This process is used
**	frequently by trees and other terrain objects. It is a good candidate for
**	optimization.
*/
template<class T>
class BlitTransXlat : public Blitter {
	public:
		BlitTransXlat(T const * translator) : TranslateTable(translator) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
};


template<class T>
class BlitTransXlatAlpha : public Blitter {
	public:
		BlitTransXlatAlpha(T const * translator, int arg) : TranslateTable(translator), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~BlitTransXlatAlpha(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
};


template<class T>
class BlitTransXlatAlphaZRead : public Blitter {
	public:
		BlitTransXlatAlphaZRead(T const * translator, int arg) : TranslateTable(translator), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~BlitTransXlatAlphaZRead(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
};


template<class T>
class BlitTransXlatAlphaZReadWrite : public Blitter {
	public:
		BlitTransXlatAlphaZReadWrite(T const * translator, int arg) : TranslateTable(translator), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~BlitTransXlatAlphaZReadWrite(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
};


template<class T>
class BlitTransXlatMultWriteAlpha : public Blitter {
	public:
		BlitTransXlatMultWriteAlpha(void) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;
};


template<class T>
class BlitTransXlatWriteAlpha : public Blitter {
	public:
		BlitTransXlatWriteAlpha(void) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;
};


template<class T>
class BlitTransXlatZRead : public Blitter {
	public:
		BlitTransXlatZRead(T const * translator) : TranslateTable(translator) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
};


template<class T>
class BlitTransXlatZReadWrite : public Blitter {
	public:
		BlitTransXlatZReadWrite(T const * translator) : TranslateTable(translator) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
};


/*
**	Blits with source 8 bit, transparency check, then translate to pixel format T. This
**	is occasionally used to render special remapping effects. Since the remap table is
**	not doubly indirected, it is fixed to only using the remap table specified in the
**	constructor. As such, it has limited value.
*/
template<class T>
class BlitTransRemapXlat : public Blitter {
	public:
		BlitTransRemapXlat(unsigned char const * remapper, T const * translator) : RemapTable(remapper), TranslateTable(translator) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		unsigned char const * RemapTable;
		T const * TranslateTable;
};


/*
**	Blits with source 8 bit with transparency then remap and dest is T. This is probably
**	the most used blitter process. Units, infantry, buildings, and aircraft use this for
**	their normal drawing needs. If any blitter process is to be optimized, this would be
**	the one. Take note that the remapper table is doubly indirected. This allows a single
**	blitter object to dynamically use alternate remap tables.
*/
template<class T>
class BlitTransZRemapXlat : public Blitter {
	public:
		BlitTransZRemapXlat(unsigned char const * const * remapper, T const * translator) : RemapTable(remapper), TranslateTable(translator) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		unsigned char const * const * RemapTable;
		T const * TranslateTable;
};


template<class T>
class BlitTransZRemapXlatAlpha : public Blitter {
	public:
		BlitTransZRemapXlatAlpha(unsigned char const * const * remapper, T const * translator, int arg) : RemapTable(remapper), TranslateTable(translator), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~BlitTransZRemapXlatAlpha(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		unsigned char const * const * RemapTable;
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
};


template<class T>
class BlitTransZRemapXlatAlphaZRead : public Blitter {
	public:
		BlitTransZRemapXlatAlphaZRead(unsigned char const * const * remapper, T const * translator, int arg) : RemapTable(remapper), TranslateTable(translator), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~BlitTransZRemapXlatAlphaZRead(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		unsigned char const * const * RemapTable;
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
};



template<class T>
class BlitTransZRemapXlatAlphaZReadWrite : public Blitter {
	public:
		BlitTransZRemapXlatAlphaZReadWrite(unsigned char const * const * remapper, T const * translator, int arg) : RemapTable(remapper), AlphaLightingRemap(NULL), TranslateTable(translator) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~BlitTransZRemapXlatAlphaZReadWrite(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		unsigned char const * const * RemapTable;
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
};


template<class T>
class BlitTransZRemapXlatZRead : public Blitter {
	public:
		BlitTransZRemapXlatZRead(unsigned char const * const * remapper, T const * translator) : RemapTable(remapper), TranslateTable(translator) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		unsigned char const * const * RemapTable;
		T const * TranslateTable;
};


template<class T>
class BlitTransZRemapXlatZReadWrite : public Blitter {
	public:
		BlitTransZRemapXlatZReadWrite(unsigned char const * const * remapper, T const * translator) : RemapTable(remapper), TranslateTable(translator) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		unsigned char const * const * RemapTable;
		T const * TranslateTable;
};


/*
**	Algorithmic darkening of hicolor pixels controlled by the source pixels. The source
**	pixels are examined only to determine if the destination pixel should be darkened.
**	If the source pixel is transparent, then the dest pixel is skipped. The darkening
**	algorithm works only for hicolor pixels.
*/
template<class T>
class BlitTransDarken : public Blitter {
	public:
		BlitTransDarken(T mask) : Mask(mask) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T Mask;
};


template<class T>
class BlitTransDarkenZRead : public Blitter {
	public:
		BlitTransDarkenZRead(T mask) : Mask(mask) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T Mask;
};


template<class T>
class BlitTransDarkenZReadWrite : public Blitter {
	public:
		BlitTransDarkenZReadWrite(T mask) : Mask(mask) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T Mask;
};


/*
**	This will remap the destination pixels but under the control of the source pixels.
**	Where the source pixel is not transparent, the dest pixel is remapped. This algorithm
**	really only applies to lowcolor display.
*/
template<class T>
class BlitTransRemapDest : public Blitter {
	public:
		BlitTransRemapDest(T const * remap) : RemapTable(remap) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * RemapTable;
};


/*
**	This is similar to BlitTransDarken but instead of examining the source to determine what
**	pixels should be darkened, every destination pixel is darkened. This means that the source
**	pointer is unused.
*/
template<class T>
class BlitDarken : public Blitter {
	public:
		BlitDarken(T mask) : Mask(mask) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T Mask;
};


/*
**	This blitter performs 50% translucency as it draws. It is commonly used for animation
**	effects and other stealth like images. It only works with hicolor pixels but is a good
**	candidate for optimization.
*/
template<class T>
class BlitTransLucent50 : public Blitter {
	public:
		BlitTransLucent50(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


template<class T>
class BlitTransLucent50Alpha : public Blitter {
	public:
		BlitTransLucent50Alpha(T const * translator, int arg, T mask) : TranslateTable(translator), AlphaLightingRemap(NULL), Mask(mask) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~BlitTransLucent50Alpha(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};


template<class T>
class BlitTransLucent50AlphaZRead : public Blitter {
	public:
		BlitTransLucent50AlphaZRead(T const * translator, int arg, T mask) : TranslateTable(translator), AlphaLightingRemap(NULL), Mask(mask) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~BlitTransLucent50AlphaZRead(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};


template<class T>
class BlitTransLucent50AlphaZReadWarp : public Blitter {
	public:
		BlitTransLucent50AlphaZReadWarp(T const * translator, int arg, T mask) : TranslateTable(translator), AlphaLightingRemap(NULL), Mask(mask) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~BlitTransLucent50AlphaZReadWarp(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};


template<class T>
class BlitTransLucent50AlphaZReadWrite : public Blitter {
	public:
		BlitTransLucent50AlphaZReadWrite(T const * translator, int arg, T mask) : TranslateTable(translator), AlphaLightingRemap(NULL), Mask(mask) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~BlitTransLucent50AlphaZReadWrite(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};


template<class T>
class BlitTransLucent50ZRead : public Blitter {
	public:
		BlitTransLucent50ZRead(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


template<class T>
class BlitTransLucent50ZReadWarp : public Blitter {
	public:
		BlitTransLucent50ZReadWarp(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


template<class T>
class BlitTransLucent50ZReadWrite : public Blitter {
	public:
		BlitTransLucent50ZReadWrite(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


template<class T>
class BlitTranslucent50NonzeroAlpha : public Blitter {
	public:
		BlitTranslucent50NonzeroAlpha(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


template<class T>
class BlitTranslucent50ZeroAlpha : public Blitter {
	public:
		BlitTranslucent50ZeroAlpha(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


/*
**	This blitter performs 25% translucency as it draws. This effect is less than spectacular,
**	but there are some uses for it. It only works with hicolor pixels.
*/
template<class T>
class BlitTransLucent25 : public Blitter {
	public:
		BlitTransLucent25(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


template<class T>
class BlitTransLucent25Alpha : public Blitter {
	public:
		BlitTransLucent25Alpha(T const * translator, int arg, T mask) : TranslateTable(translator), AlphaLightingRemap(NULL), Mask(mask) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~BlitTransLucent25Alpha(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};


template<class T>
class BlitTransLucent25AlphaZRead : public Blitter {
	public:
		BlitTransLucent25AlphaZRead(T const * translator, int arg, T mask) : TranslateTable(translator), AlphaLightingRemap(NULL), Mask(mask) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~BlitTransLucent25AlphaZRead(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};


template<class T>
class BlitTransLucent25AlphaZReadWarp : public Blitter {
	public:
		BlitTransLucent25AlphaZReadWarp(T const * translator, int arg, T mask) : TranslateTable(translator), AlphaLightingRemap(NULL), Mask(mask) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~BlitTransLucent25AlphaZReadWarp(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};


template<class T>
class BlitTransLucent25AlphaZReadWrite : public Blitter {
	public:
		BlitTransLucent25AlphaZReadWrite(T const * translator, int arg, T mask) : TranslateTable(translator), AlphaLightingRemap(NULL), Mask(mask) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~BlitTransLucent25AlphaZReadWrite(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};



template<class T>
class BlitTransLucent25ZRead : public Blitter {
	public:
		BlitTransLucent25ZRead(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


template<class T>
class BlitTransLucent25ZReadWarp : public Blitter {
	public:
		BlitTransLucent25ZReadWarp(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


template<class T>
class BlitTransLucent25ZReadWrite : public Blitter {
	public:
		BlitTransLucent25ZReadWrite(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


/*
**	This blitter performs 75% translucency as it draws. This is quite useful for explosions and
**	other gas animation effects. It only works with hicolor pixels and is a good candidate
**	for optimization.
*/
template<class T>
class BlitTransLucent75 : public Blitter {
	public:
		BlitTransLucent75(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


template<class T>
class BlitTransLucent75Alpha : public Blitter {
	public:
		BlitTransLucent75Alpha(T const * translator, int arg, T mask) : TranslateTable(translator), AlphaLightingRemap(NULL), Mask(mask) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~BlitTransLucent75Alpha(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};


template<class T>
class BlitTransLucent75AlphaZRead : public Blitter {
	public:
		BlitTransLucent75AlphaZRead(T const * translator, int arg, T mask) : TranslateTable(translator), AlphaLightingRemap(NULL), Mask(mask) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~BlitTransLucent75AlphaZRead(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};


template<class T>
class BlitTransLucent75AlphaZReadWarp : public Blitter {
	public:
		BlitTransLucent75AlphaZReadWarp(T const * translator, int arg, T mask) : TranslateTable(translator), AlphaLightingRemap(NULL), Mask(mask) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~BlitTransLucent75AlphaZReadWarp(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};


template<class T>
class BlitTransLucent75AlphaZReadWrite : public Blitter {
	public:
		BlitTransLucent75AlphaZReadWrite(T const * translator, int arg, T mask) : TranslateTable(translator), AlphaLightingRemap(NULL), Mask(mask) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~BlitTransLucent75AlphaZReadWrite(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};


template<class T>
class BlitTransLucent75ZRead : public Blitter {
	public:
		BlitTransLucent75ZRead(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


template<class T>
class BlitTransLucent75ZReadWarp : public Blitter {
	public:
		BlitTransLucent75ZReadWarp(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


template<class T>
class BlitTransLucent75ZReadWrite : public Blitter {
	public:
		BlitTransLucent75ZReadWrite(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};



template<class T>
class BlitTranslucent75NonzeroAlpha : public Blitter {
	public:
		BlitTranslucent75NonzeroAlpha(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


template<class T>
class BlitTranslucent75ZeroAlpha : public Blitter {
	public:
		BlitTranslucent75ZeroAlpha(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


template<class T>
class BlitTranslucentWriteAlpha : public Blitter {
	public:
		BlitTranslucentWriteAlpha(T const * translator) : TranslateTable(translator) {}
		virtual void BlitForward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset) const override;

		/*
		**	The backward moving method will probably never be called in actual practice.
		**	Implement in terms of the forward copying method until the need for this
		**	version arrises.
		*/
		virtual void BlitBackward(void* dest, void const* source, int length, int z_min, void* z_buff, void* a_buff, int alpha_level) const override;

	protected:
		T const * TranslateTable;
};


/*
**	Assembly versions of some of the templated blitter object functions. Borland and 
**	Visual C++ support a compatible inline-assembly formats. However, Borland compiler
**	does not allow inline-assembly to be part of an inline function -- go figure. 
**	It will still compile, it just generates warning messages.
*/

/* ===== RLE-compressed blitters ===== */

template<class T>
class RLEBlitTransXlat : public RLEBlitter {
	public:
		RLEBlitTransXlat(T const * translator) : TranslateTable(translator) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
};


template<class T>
class RLEBlitTransXlatAlpha : public RLEBlitter
{
	public:
		RLEBlitTransXlatAlpha(T const * translator, int arg) : TranslateTable(translator), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~RLEBlitTransXlatAlpha(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
};


template<class T>
class RLEBlitTransXlatAlphaZRead : public RLEBlitter
{
	public:
		RLEBlitTransXlatAlphaZRead(T const * translator, int arg) : TranslateTable(translator), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~RLEBlitTransXlatAlphaZRead(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
};


template<class T>
class RLEBlitTransXlatAlphaZReadWrite : public RLEBlitter
{
	public:
		RLEBlitTransXlatAlphaZReadWrite(T const * translator, int arg) : TranslateTable(translator), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~RLEBlitTransXlatAlphaZReadWrite(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
};

template<class T>
class RLEBlitTransXlatZRead : public RLEBlitter
{
	public:
		RLEBlitTransXlatZRead(T const * translator) : TranslateTable(translator) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
};


template<class T>
class RLEBlitTransXlatZReadWrite : public RLEBlitter
{
	public:
		RLEBlitTransXlatZReadWrite(T const * translator) : TranslateTable(translator) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
};


/*
**	This blits RLE compressed pixels by first remapping through a 256 byte table and then
**	translating the pixel to screen format.
*/
template<class T>
class RLEBlitTransRemapXlat : public RLEBlitter {
	public:
		RLEBlitTransRemapXlat(unsigned char const * remapper, T const * translator) : RemapTable(remapper), TranslateTable(translator) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		unsigned char const * RemapTable;
		T const * TranslateTable;
};


template<class T>
class RLEBlitTransRemapXlatZRead : public RLEBlitter
{
	public:
		RLEBlitTransRemapXlatZRead(unsigned char const * remapper, T const * translator) : RemapTable(remapper), TranslateTable(translator) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		unsigned char const * RemapTable;
		T const * TranslateTable;
};



template<class T>
class RLEBlitTransRemapXlatZReadWrite : public RLEBlitter
{
	public:
		RLEBlitTransRemapXlatZReadWrite(unsigned char const * remapper, T const * translator) : RemapTable(remapper), TranslateTable(translator) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		unsigned char const * RemapTable;
		T const * TranslateTable;
};


/*
**	This blits RLE compressed pixels by first remapping through a 256 byte table and then
**	translating the pixel to screen format. The remapping table is doubly indirected so that
**	it is possible to change the remapping table pointer without creating a separate blitter
**	object.
*/
template<class T>
class RLEBlitTransZRemapXlat : public RLEBlitter {
	public:
		RLEBlitTransZRemapXlat(unsigned char const * const * remapper, T const * translator) : RemapTable(remapper), TranslateTable(translator) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		unsigned char const * const * RemapTable;
		T const * TranslateTable;
};


template<class T>
class RLEBlitTransZRemapXlatAlpha : public RLEBlitter
{
	public:
		RLEBlitTransZRemapXlatAlpha(unsigned char const * const * remapper, T const * translator, int arg) : RemapTable(remapper), TranslateTable(translator), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~RLEBlitTransZRemapXlatAlpha(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		unsigned char const * const * RemapTable;
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
};


template<class T>
class RLEBlitTransZRemapXlatAlphaZRead : public RLEBlitter
{
	public:
		RLEBlitTransZRemapXlatAlphaZRead(unsigned char const * const * remapper, T const * translator, int arg) : RemapTable(remapper), TranslateTable(translator), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~RLEBlitTransZRemapXlatAlphaZRead(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		unsigned char const * const * RemapTable;
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
};


template<class T>
class RLEBlitTransZRemapXlatAlphaZReadWrite : public RLEBlitter
{
	public:
		RLEBlitTransZRemapXlatAlphaZReadWrite(unsigned char const * const * remapper, T const * translator, int arg) : RemapTable(remapper), TranslateTable(translator), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~RLEBlitTransZRemapXlatAlphaZReadWrite(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		unsigned char const * const * RemapTable;
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
};


template<class T>
class RLEBlitTransZRemapXlatZRead : public RLEBlitter
{
	public:
		RLEBlitTransZRemapXlatZRead(unsigned char const * const * remapper, T const * translator) : RemapTable(remapper), TranslateTable(translator) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		unsigned char const * const * RemapTable;
		T const * TranslateTable;
};

template<class T>
class RLEBlitTransZRemapXlatZReadWrite : public RLEBlitter
{
	public:
		RLEBlitTransZRemapXlatZReadWrite(unsigned char const * const * remapper, T const * translator) : RemapTable(remapper), TranslateTable(translator) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		unsigned char const * const * RemapTable;
		T const * TranslateTable;
};


/*
**	This will remap the destination pixels but under the control of the source pixels.
**	Where the source pixel is not transparent, the dest pixel is remapped. This algorithm
**	really only applies to lowcolor display.
*/
template<class T>
class RLEBlitTransRemapDest : public RLEBlitter {
	public:
		RLEBlitTransRemapDest(T const * remap) : RemapTable(remap) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * RemapTable;
};


template<class T>
class RLEBlitTransRemapDestZRead : public RLEBlitter
{
	public:
		RLEBlitTransRemapDestZRead(T const * remap) : RemapTable(remap) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * RemapTable;
};


template<class T>
class RLEBlitTransRemapDestZReadWrite : public RLEBlitter
{
	public:
		RLEBlitTransRemapDestZReadWrite(T const * remap) : RemapTable(remap) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * RemapTable;
};


/*
**	Algorithmic darkening of hicolor pixels controlled by the source pixels. The source
**	pixels are examined only to determine if the destination pixel should be darkened.
**	If the source pixel is transparent, then the dest pixel is skipped. The darkening
**	algorithm works only for hicolor pixels.
*/
template<class T>
class RLEBlitTransDarken : public RLEBlitter {
	public:
		RLEBlitTransDarken(T mask) : Mask(mask) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T Mask;
};


template<class T>
class RLEBlitTransDarkenZRead : public RLEBlitter
{
	public:
		RLEBlitTransDarkenZRead(T mask) : Mask(mask) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T Mask;
};


template<class T>
class RLEBlitTransDarkenZReadWrite : public RLEBlitter
{
	public:
		RLEBlitTransDarkenZReadWrite(T mask) : Mask(mask) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T Mask;
};


/*
**	This blitter performs 50% translucency as it draws. It is commonly used for animation
**	effects and other stealth like images. It only works with hicolor pixels but is a good
**	candidate for optimization.
*/
template<class T>
class RLEBlitTransLucent50 : public RLEBlitter {
	public:
		RLEBlitTransLucent50(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


template<class T>
class RLEBlitTransLucent50Alpha : public RLEBlitter
{
	public:
		RLEBlitTransLucent50Alpha(T const * translator, int arg, T mask) : TranslateTable(translator), Mask(mask), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~RLEBlitTransLucent50Alpha(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};


template<class T>
class RLEBlitTransLucent50AlphaZRead : public RLEBlitter
{
	public:
		RLEBlitTransLucent50AlphaZRead(T const * translator, int arg, T mask) : TranslateTable(translator), Mask(mask), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~RLEBlitTransLucent50AlphaZRead(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};


template<class T>
class RLEBlitTransLucent50AlphaZReadWarp : public RLEBlitter
{
	public:
		RLEBlitTransLucent50AlphaZReadWarp(T const * translator, int arg, T mask) : TranslateTable(translator), Mask(mask), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~RLEBlitTransLucent50AlphaZReadWarp(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};


template<class T>
class RLEBlitTransLucent50AlphaZReadWrite : public RLEBlitter
{
	public:
		RLEBlitTransLucent50AlphaZReadWrite(T const * translator, int arg, T mask) : TranslateTable(translator), Mask(mask), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~RLEBlitTransLucent50AlphaZReadWrite(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};


template<class T>
class RLEBlitTransLucent50ZRead : public RLEBlitter
{
	public:
		RLEBlitTransLucent50ZRead(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


template<class T>
class RLEBlitTransLucent50ZReadWarp : public RLEBlitter
{
	public:
		RLEBlitTransLucent50ZReadWarp(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


template<class T>
class RLEBlitTransLucent50ZReadWrite : public RLEBlitter
{
	public:
		RLEBlitTransLucent50ZReadWrite(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


/*
**	This blitter performs 25% translucency as it draws. This effect is less than spectacular,
**	but there are some uses for it. It only works with hicolor pixels.
*/
template<class T>
class RLEBlitTransLucent25 : public RLEBlitter {
	public:
		RLEBlitTransLucent25(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


template<class T>
class RLEBlitTransLucent25Alpha : public RLEBlitter
{
	public:
		RLEBlitTransLucent25Alpha(T const * translator, int arg, T mask) : TranslateTable(translator), Mask(mask), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~RLEBlitTransLucent25Alpha(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};


template<class T>
class RLEBlitTransLucent25AlphaZRead : public RLEBlitter
{
	public:
		RLEBlitTransLucent25AlphaZRead(T const * translator, int arg, T mask) : TranslateTable(translator), Mask(mask), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~RLEBlitTransLucent25AlphaZRead(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};


template<class T>
class RLEBlitTransLucent25AlphaZReadWarp : public RLEBlitter
{
	public:
		RLEBlitTransLucent25AlphaZReadWarp(T const * translator, int arg, T mask) : TranslateTable(translator), Mask(mask), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~RLEBlitTransLucent25AlphaZReadWarp(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};


template<class T>
class RLEBlitTransLucent25AlphaZReadWrite : public RLEBlitter
{
	public:
		RLEBlitTransLucent25AlphaZReadWrite(T const * translator, int arg, T mask) : TranslateTable(translator), Mask(mask), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~RLEBlitTransLucent25AlphaZReadWrite(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};


template<class T>
class RLEBlitTransLucent25ZRead : public RLEBlitter
{
	public:
		RLEBlitTransLucent25ZRead(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


template<class T>
class RLEBlitTransLucent25ZReadWarp : public RLEBlitter
{
	public:
		RLEBlitTransLucent25ZReadWarp(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


template<class T>
class RLEBlitTransLucent25ZReadWrite : public RLEBlitter
{
	public:
		RLEBlitTransLucent25ZReadWrite(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


/*
**	This blitter performs 75% translucency as it draws. This is quite useful for explosions and
**	other gas animation effects. It only works with hicolor pixels and is a good candidate
**	for optimization.
*/
template<class T>
class RLEBlitTransLucent75 : public RLEBlitter {
	public:
		RLEBlitTransLucent75(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


template<class T>
class RLEBlitTransLucent75Alpha : public RLEBlitter
{
	public:
		RLEBlitTransLucent75Alpha(T const * translator, int arg, T mask) : TranslateTable(translator), Mask(mask), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~RLEBlitTransLucent75Alpha(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};


template<class T>
class RLEBlitTransLucent75AlphaZRead : public RLEBlitter
{
	public:
		RLEBlitTransLucent75AlphaZRead(T const * translator, int arg, T mask) : TranslateTable(translator), Mask(mask), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~RLEBlitTransLucent75AlphaZRead(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};


template<class T>
class RLEBlitTransLucent75AlphaZReadWarp : public RLEBlitter
{
	public:
		RLEBlitTransLucent75AlphaZReadWarp(T const * translator, int arg, T mask) : TranslateTable(translator), Mask(mask), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~RLEBlitTransLucent75AlphaZReadWarp(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};


template<class T>
class RLEBlitTransLucent75AlphaZReadWrite : public RLEBlitter
{
	public:
		RLEBlitTransLucent75AlphaZReadWrite(T const * translator, int arg, T mask) : TranslateTable(translator), Mask(mask), AlphaLightingRemap(NULL) {AlphaLightingRemap = AlphaLightingRemapInit.Init(arg);}
		virtual ~RLEBlitTransLucent75AlphaZReadWrite(void) override { AlphaLightingRemapInit.Deinit(AlphaLightingRemap); AlphaLightingRemap = NULL; }

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		AlphaLightingRemapClass *AlphaLightingRemap;
		T Mask;
};


template<class T>
class RLEBlitTransLucent75ZRead : public RLEBlitter
{
	public:
		RLEBlitTransLucent75ZRead(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};

template<class T>
class RLEBlitTransLucent75ZReadWarp : public RLEBlitter
{
	public:
		RLEBlitTransLucent75ZReadWarp(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};

template<class T>
class RLEBlitTransLucent75ZReadWrite : public RLEBlitter
{
	public:
		RLEBlitTransLucent75ZReadWrite(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}

		virtual void Blit(void* dest, void const* source, int length, int leadskip, int z_min, void* z_buff, void* a_buff, int alpha_level, int warp_offset, void const* zshape) const override;

	protected:
		T const * TranslateTable;
		T Mask;
};


