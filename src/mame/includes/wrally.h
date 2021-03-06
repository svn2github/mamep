class wrally_state : public driver_device
{
public:
	wrally_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_videoram(*this, "videoram"),
		m_vregs(*this, "vregs"),
		m_spriteram(*this, "spriteram"),
		m_shareram(*this, "shareram"),
		m_maincpu(*this, "maincpu"),
		m_gfxdecode(*this, "gfxdecode"),
		m_palette(*this, "palette") { }

	tilemap_t *m_pant[2];
	required_shared_ptr<UINT16> m_videoram;
	required_shared_ptr<UINT16> m_vregs;
	required_shared_ptr<UINT16> m_spriteram;
	required_shared_ptr<UINT16> m_shareram;

	DECLARE_READ8_MEMBER(dallas_share_r);
	DECLARE_WRITE8_MEMBER(dallas_share_w);
	DECLARE_WRITE16_MEMBER(wrally_vram_w);
	DECLARE_WRITE16_MEMBER(wrally_flipscreen_w);
	DECLARE_WRITE16_MEMBER(OKIM6295_bankswitch_w);
	DECLARE_WRITE16_MEMBER(wrally_coin_counter_w);
	DECLARE_WRITE16_MEMBER(wrally_coin_lockout_w);
	TILE_GET_INFO_MEMBER(get_tile_info_wrally_screen0);
	TILE_GET_INFO_MEMBER(get_tile_info_wrally_screen1);
	virtual void video_start();
	UINT32 screen_update_wrally(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	void draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect, int priority);
	required_device<cpu_device> m_maincpu;
	required_device<gfxdecode_device> m_gfxdecode;
	required_device<palette_device> m_palette;
};
