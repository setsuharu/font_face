#pragma once
#include <vector>
#include <algorithm>
#include <string>
#include <map>
#include <unordered_map>
#include "util.hpp"
#include "bitmap/bitmap.hpp"

namespace tou
{
	tou::ivec2 calculate_point_at_t(float t, float x1, float x2, float y1, float y2, bool bezier, float cx = 0.0f, float cy = 0.0f);

	namespace truetype
	{
		struct offset_table
		{
			uint32_t sfnt_version = 0;
			uint16_t num_tables = 0;
			uint16_t search_range = 0;
			uint16_t entry_selector = 0;
			uint16_t range_shift = 0;
		};

		struct table_record
		{
			uint32_t tag = 0;
			uint32_t checksum = 0;
			uint32_t offset = 0;
			uint32_t length = 0;
		};

		struct head
		{
			uint16_t major_version = 0, minor_version = 0;
			int32_t font_revision = 0;
			uint32_t check_sum_adj = 0;
			uint32_t magic_number = 0x5F0F3CF5;
			uint16_t flags = 0;
			uint16_t units_per_em = 0;
			int64_t date_created = 0, date_modified = 0;
			int16_t x_min = 0, y_min = 0, x_max = 0, y_max = 0;
			uint16_t mac_style = 0, lowest_rec_ppem = 0;
			int16_t font_direction_hint = 0; // deprecated
			int16_t index_to_loc_format = 0; // determines which loca table is used 'short' or 'long'
			int16_t glyph_data_format = 0;
		};

		struct maxp
		{
			uint32_t version = 0;
			uint16_t num_glyphs = 0, max_points = 0, max_contours = 0, max_composite_points = 0, max_composite_contours = 0, max_zones = 0, max_twilight_points = 0;
			uint16_t max_storage = 0, max_function_defs = 0, max_instruction_defs = 0, max_stack_elements = 0, max_size_of_instructions = 0;
			uint16_t max_component_elements = 0, max_component_depth = 0;
		};

		struct cmap_encoding
		{
			uint16_t platform_id = 0, encoding_id = 0;
			uint32_t offset = 0; // offset to start of subtable from beginning of this table
		};

		struct cmap_header
		{
			uint16_t version = 0, num_tables = 0;
			std::vector<cmap_encoding> encoding_records; // has numTables elements
		};

		struct cmap_format4
		{
			uint16_t format = 0, length = 0, language = 0, seg_count_x2 = 0, search_range = 0, entry_selector = 0, range_shift = 0;
			std::vector<uint16_t> end_code;			// segCount elements
			std::vector<uint16_t> start_code;		// segCount elements
			std::vector<int16_t> id_delta;			// segCount elements
			std::vector<uint16_t> id_range_offset;	// segCount elements, offsets from here to glyphIdArray or 0
			std::vector<uint16_t> glyph_id_array;
		};

		/* hmtx table */
		struct long_hor_metric
		{
			// these are in font design units
			uint16_t advance_width = 0;
			int16_t lsb = 0; // glyph left side bearing
		};

		struct hmtx
		{
			// records are indexed by glyph id
			std::vector<long_hor_metric> hmetrics;
			std::vector<int16_t> left_side_bearings;
		};

		/* hhea table */
		struct hhea
		{
			uint16_t major_ver = 0, minor_ver = 0;
			int16_t ascender = 0, descender = 0, line_gap = 0; // font design units (these values are Apple specific)
			uint16_t advance_width_max = 0; // font design units
			int16_t min_left_side_bearing = 0, min_right_side_bearing = 0, x_max_extent = 0; // font design units
			int16_t caret_slope_rise = 0, caret_slope_run = 0, caret_offset = 0;
			// 8 byte gap containing 0s
			int16_t metric_data_format = 0;
			uint16_t number_of_hmetrics = 0; // number of hMetric entries in htmx table

			// TO DO: hhea/OpenType font variations
		};

		struct glyph_flags
		{
			bool on_curve_point = false;
			bool x_short_vector = false, y_short_vector = false;
			bool repeat_flag = false;
			uint8_t repeat_count = 0;
			bool x_is_same_or_positive_x_short_vector = false, y_is_same_or_positive_y_short_vector = false;
			bool overlap_simple = false;
		};

		struct glyph_component
		{
			// used for composite glyphs
			uint16_t flag = 0;
			uint16_t glyph_index = 0;
			int16_t xy_arg1 = 0, xy_arg2 = 0;
			uint16_t pt_arg1 = 0, pt_arg2 = 0;
			uint16_t scale = 0, x_scale = 0, y_scale = 0, scale01 = 0, scale10 = 0;
			bool round_to_nearest_grid_line = false;
			bool instructions_present = false;
			bool use_base_glyph_aw_and_lsb = false;
			bool overlap_compound = false;
			bool scaled_component_offset = false;
		};
	}

	struct glyph_value
	{
		uint32_t f26;
		float vectorial;

		operator unsigned int()
		{
			return static_cast<unsigned int>(f26);
		}

		bool operator==(const glyph_value& rhs)
		{
			return (this->f26 == rhs.f26 && this->vectorial == rhs.vectorial);
		}

		bool operator!=(const glyph_value& rhs)
		{
			return !(*this == rhs);
		}
	};

	struct glyph_outline_value
	{
		glyph_value value;
		int associated_segment_id = -1;
		bool member_of_on_segment = false;
		bool part_of_bezier = false;
		bool part_of_horizontal = false;

		// compares f26 values only
		bool operator==(const glyph_outline_value& rhs)
		{
			return ((this->value.f26 == rhs.value.f26) && (this->associated_segment_id == rhs.associated_segment_id) &&
				(this->member_of_on_segment == rhs.member_of_on_segment) && (this->part_of_bezier == rhs.part_of_bezier) &&
				(this->part_of_horizontal == rhs.part_of_horizontal));
		}

	};

	struct glyph_outline_segment
	{
		tou::ivec2 start{ 0, 0 };	// p1
		tou::ivec2 end{ 0, 0 };		// p2
		tou::ivec2 control{ 0, 0 };	// only if bezier = true
		bool bezier = false;
		bool denotes_on_transition = false;	// true if y is increasing from start to end, false if y is decreasing
		bool horizontal = false;	// y does not change and the segment is not a bezier
		bool vertical = false;		// x does not change and the segment is not a bezier
		int direction = -1;			// 0 for right to left, 1 for left to right (-1 if vertical)
		std::map<uint32_t, std::vector<glyph_value>> values;
		//std::map<uint32_t, float> y_values;
	};

	//struct glyph_outline
	//{
	//	std::map<uint32_t, std::vector<glyph_outline_value>> outline;
	//};

	struct glyph_bounding_box
	{
		uint32_t x_min = 0, x_max = 0, y_min = 0, y_max = 0;
	};

	class font_face
	{
	public:
		struct truetype_glyph
		{
			// vectorial values
			uint16_t id = 0;
			int16_t x_min = 0, y_min = 0, x_max = 0, y_max = 0;
			int16_t num_contours = 0; // -1 is for composite glyphs
			std::vector<uint16_t> end_pts_of_contours;	
			uint16_t num_points = 0;
			uint16_t instruction_len = 0;
			std::vector<uint8_t> instructions;
			uint16_t advance_width = 0;
			int16_t left_side_bearing = 0;
			std::vector<truetype::glyph_flags> flags_bool;
			std::vector<int16_t> x_coords, y_coords;
		};

		struct bitmap_glyph
		{
			// expressed as pixel values derived from 26.6 fixed float format
			uint16_t id = 0;
			uint32_t advance_x = 0;
			tou::bitmap_image image;
		};

	public:
		font_face();
		font_face(const std::string& filepath);
		~font_face();

		bool load(const std::string& filepath);
		const font_face::truetype_glyph& get_glyph(uint16_t unicode);
		font_face::bitmap_glyph get_glyph_bitmap(uint16_t unicode, float pointsize, bool render_outline, bool render_inside);
		
		bool ok() const { return m_ok; }
		explicit operator bool() const { return m_ok; }

	private:
		bool m_parse_truetype_file(const std::string& filepath);
		
		uint16_t m_get_truetype_glyph_id(uint16_t unicode);
		bool m_get_truetype_simple_glyph_header_data(font_face::truetype_glyph& glyph);
		void m_get_truetype_simple_glyph_data(font_face::truetype_glyph& glyph);
		void m_get_truetype_component_glyph_data(std::vector<truetype::glyph_component>& components);
		void m_get_truetype_glyph_data_by_id(font_face::truetype_glyph& glyph, uint16_t glyph_id);
		void m_get_truetype_glyph_data_by_id(font_face::truetype_glyph& glyph, uint16_t glyph_id, std::vector<truetype::glyph_component>& components);
		
		font_face::truetype_glyph m_get_truetype_glyph(uint16_t unicode);
		
		font_face::bitmap_glyph m_rasterize_truetype_glyph(const font_face::truetype_glyph& g, float pointsize, bool render_outline, bool render_inside);

	private:
		tou::vector_reader													m_reader;
		std::unordered_map<std::string, tou::truetype::table_record>		m_table_records;
		std::pair<tou::truetype::cmap_header, tou::truetype::cmap_format4>	m_cmap;
		tou::truetype::hmtx													m_hmtx;
		std::vector<uint32_t>												m_loca_offset32;
		std::map<uint16_t, font_face::truetype_glyph>						m_glyphs; // only contains glyphs queried for by user
		
		bool		m_ok;
		uint32_t	m_sfnt;
		uint16_t	m_num_glyphs;
		uint16_t	m_units_per_em;
		uint16_t	m_seg_count;
		uint64_t	m_id_range_offset_from_filestart;
	};
}
