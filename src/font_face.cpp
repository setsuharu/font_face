#include <algorithm>
#include "font_face.hpp"

#define FILL_BLK { 0x00, 0x00, 0x00, 0xFF }
#define FIND(m, y, x) std::find(m[y].begin(), m[y].end(), x)  != m[y].end()
#define FINDV(v, n) std::find(v.begin(), v.end(), n) != v.end()
#define FINDV_IT(v, n) std::find(v.begin(), v.end(), n)

namespace tou
{
	float convert_to_pixel(float value, float pointsize, float dpi, float units_per_em)
	{
		return ((value * pointsize * dpi) / (72.0f * units_per_em));
	}

	uint32_t convert_to_f26(float f)
	{
		// this function does not work if 'f' is negative
		return static_cast<uint32_t>((f * 64.0f) + (64.0f * (f - (long)f)));
	}

	uint32_t roundf26(uint32_t x)
	{
		return ((x + 32) & -64);
	}

	uint32_t floorf26(uint32_t x)
	{
		return (x & -64);
	}

	uint32_t ceilf26(uint32_t x)
	{
		return ((x + 63) & -64);
	}

	tou::ivec2 midpoint(uint32_t x1, uint32_t x2, uint32_t y1, uint32_t y2)
	{
		return { ((x1 + x2) / 2), ((y1 + y2) / 2) };
	}

	float bezier2(float t, float p1, float p2, float c)
	{
		return (((1 - t) * (1 - t)) * p1 + 2 * (1 - t) * t * c + (t * t) * p2);
	}

	float linear(float t, float p1, float p2)
	{
		return ((1 - t) * p1 + t * p2);
	}

	tou::ivec2 calculate_point_at_t(float t, float x1, float x2, float y1, float y2, bool bezier, float cx, float cy)
	{
		tou::ivec2 out{ 0, 0 };
		if (bezier)
		{
			out.x = tou::roundf26(static_cast<uint32_t>(tou::bezier2(t, x1, x2, cx)));
			out.y = tou::roundf26(static_cast<uint32_t>(tou::bezier2(t, y1, y2, cy)));
		}
		else
		{
			out.x = tou::roundf26(static_cast<uint32_t>(tou::linear(t, x1, x2)));
			out.y = tou::roundf26(static_cast<uint32_t>(tou::linear(t, y1, y2)));
		}
		return out;
	}

	tou::glyph_outline_segment calculate_segment_path(float pointsize, float dpi, float units_per_em, float x1, float x2, float y1, float y2, bool bezier, float cx, float cy)
	{
		tou::glyph_outline_segment seg;
		uint32_t f26_x1 = tou::roundf26(tou::convert_to_f26(tou::convert_to_pixel(x1, pointsize, dpi, units_per_em)));
		uint32_t f26_x2 = tou::roundf26(tou::convert_to_f26(tou::convert_to_pixel(x2, pointsize, dpi, units_per_em)));
		uint32_t f26_y1 = tou::roundf26(tou::convert_to_f26(tou::convert_to_pixel(y1, pointsize, dpi, units_per_em)));
		uint32_t f26_y2 = tou::roundf26(tou::convert_to_f26(tou::convert_to_pixel(y2, pointsize, dpi, units_per_em)));
		uint32_t f26_cx = (bezier ? tou::roundf26(tou::convert_to_f26(tou::convert_to_pixel(cx, pointsize, dpi, units_per_em))) : 0);
		uint32_t f26_cy = (bezier ? tou::roundf26(tou::convert_to_f26(tou::convert_to_pixel(cy, pointsize, dpi, units_per_em))) : 0);
		seg.start = { f26_x1, f26_y1 };
		seg.end = { f26_x2, f26_y2 };
		seg.control = { f26_cx, f26_cy };

		int difference = 0;
		difference = static_cast<int>(seg.end.y) - static_cast<int>(seg.start.y);
		if (bezier && difference == 0)
			difference = static_cast<int>(y2) - static_cast<int>(y1);
		seg.denotes_on_transition = (difference > 0);
		seg.horizontal = ((difference == 0) && !bezier);
		seg.bezier = bezier;
		
		if (pointsize < 32.0f)
		{
			for (float t = 0.00f; t <= 1.00f; t += 0.01f)
			{
				tou::ivec2 pt = tou::calculate_point_at_t(t, FLT(f26_x1), FLT(f26_x2), FLT(f26_y1), FLT(f26_y2), bezier, FLT(f26_cx), FLT(f26_cy));

				if (!(FIND(seg.values, pt.y, pt.x)))
					seg.values[pt.y].push_back(pt.x);
			}
		}
		else
		{
			for (float t = 0.001f; t <= 1.000f; t += 0.001f)
			{
				tou::ivec2 pt = tou::calculate_point_at_t(t, FLT(f26_x1), FLT(f26_x2), FLT(f26_y1), FLT(f26_y2), bezier, FLT(f26_cx), FLT(f26_cy));

				if (!(FIND(seg.values, pt.y, pt.x)))
					seg.values[pt.y].push_back(pt.x);
			}
		}
		
		return seg;
	}

	void add_to_map(glyph_outline_segment& segment, std::map<uint32_t, std::vector<uint32_t>>& outline_map)
	{
		for (auto& pair : segment.values)
		{
			for (size_t p = 0; p < pair.second.size(); p++)
			{
				if (!(FIND(outline_map, pair.first, pair.second[p])))
					outline_map[pair.first].push_back(pair.second[p]);
			}
			std::sort(outline_map[pair.first].begin(), outline_map[pair.first].end());
		}
	}

	bool interior_row(std::vector<tou::glyph_outline_segment>& outline, uint32_t y, uint32_t xstart, uint32_t xend)
	{
		// check to see if the given xstart belongs to an 'on' segment and if the given xend belongs to an 'off' segment
		// function assumes y is valid and xstart and xend exist in the outline
		bool valid_start = false, valid_end = false;
		for (auto& seg : outline)
		{
			if (seg.values.find(y) != seg.values.end())
			{
				if ((FIND(seg.values, y, xstart)))
				{
					if ((seg.denotes_on_transition) && (!seg.horizontal))
					{
						valid_start = true;
					}
					else if ((!seg.denotes_on_transition) && (!seg.horizontal) && seg.bezier)
					{
						if ((xstart >= seg.control.x) && (y < seg.end.y))
							valid_start = true;
					}
				}
				if (((FIND(seg.values, y, xend))))
				{
					if ((!seg.denotes_on_transition) && (!seg.horizontal))
						valid_end = true;
				}
			}

			if (valid_start && valid_end)
				break;
		}
		return (valid_start && valid_end);
	}

	std::vector<size_t> get_segment_index(std::vector<glyph_outline_segment>& outline, uint32_t y, uint32_t x)
	{
		std::vector<size_t> indexes;
		for (size_t i = 0; i < outline.size(); i++)
		{
			if (outline[i].values.find(y) != outline[i].values.end())
				if (FIND(outline[i].values, y, x))
					indexes.push_back(i);
		}
		return indexes;
	}

	constexpr uint8_t ON_CURVE_POINT = 0x01;
	constexpr uint8_t X_SHORT_VECTOR = 0x02;
	constexpr uint8_t Y_SHORT_VECTOR = 0x04;
	constexpr uint8_t REPEAT_FLAG = 0x08;
	constexpr uint8_t X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR = 0x10;
	constexpr uint8_t Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR = 0x20;
	constexpr uint8_t OVERLAP_SIMPLE = 0x40;

	constexpr uint16_t ARG_1_AND_2_ARE_WORDS = 0x0001;
	constexpr uint16_t ARGS_ARE_XY_VALUES = 0x0002;
	constexpr uint16_t ROUND_XY_TO_GRID = 0x0004;
	constexpr uint16_t WE_HAVE_A_SCALE = 0x0008;
	constexpr uint16_t MORE_COMPONENTS = 0x0020;
	constexpr uint16_t WE_HAVE_AN_X_AND_Y_SCALE = 0x0040;
	constexpr uint16_t WE_HAVE_A_TWO_BY_TWO = 0x0080;
	constexpr uint16_t WE_HAVE_INSTRUCTIONS = 0x0100;
	constexpr uint16_t USE_MY_METRICS = 0x0200;
	constexpr uint16_t OVERLAP_COMPOUND = 0x0400;
	constexpr uint16_t SCALED_COMPONENT_OFFSET = 0x0800;
	constexpr uint16_t UNSCALED_COMPONENT_OFFSET = 0x1000;

	font_face::font_face()
		:m_ok(false), m_sfnt(0x00010000), m_num_glyphs(0), m_units_per_em(0), m_seg_count(0), m_id_range_offset_from_filestart(0)
	{
	}

	font_face::font_face(const std::string& filepath)
		: m_ok(false), m_sfnt(0x00010000), m_num_glyphs(0), m_units_per_em(0),  m_seg_count(0), m_id_range_offset_from_filestart(0)
	{
		m_ok = load(filepath);
	}

	font_face::~font_face()
	{
	}

	bool font_face::load(const std::string& filepath)
	{
		// this function will eventually include logic to determine what kind of file has been passed
		// for now, we assume a ttf file has been provided for parsing
		m_ok = m_parse_truetype_file(filepath);
		return m_ok;
	}

	const font_face::truetype_glyph& font_face::get_glyph(uint16_t unicode)
	{
		if (m_glyphs.find(unicode) != m_glyphs.end())
		{
			return m_glyphs[unicode];
		}
		else
		{
			font_face::truetype_glyph glyph = m_get_truetype_glyph(unicode);
			if (glyph.id != 0)
			{
				m_glyphs.insert({ unicode, glyph });
				return m_glyphs[unicode];
			}
			else
			{
				LOG("The requested glyph could not be found in the font file");
				if (m_glyphs.find(0) != m_glyphs.end())
				{
					return m_glyphs[0];
				}
				else
				{
					m_glyphs.insert({ 0, glyph });
					return m_glyphs[0];
				}
			}
		}
		return m_glyphs[0];
	}

	font_face::bitmap_glyph font_face::get_glyph_bitmap(uint16_t unicode, float point_size, bool render_outline, bool render_inside)
	{
		font_face::truetype_glyph g = get_glyph(unicode);

#ifdef _DEBUG
		if (g.id == 0) LOG("An empty glyph was returned as a bitmap");
#endif

		return m_rasterize_truetype_glyph(g, point_size, render_outline, render_inside);
	}

	bool font_face::m_parse_truetype_file(const std::string& filepath)
	{
		if (!m_reader.load(filepath))
			return false;

		tou::truetype::offset_table offset_table;
		offset_table.sfnt_version = m_reader.get_uint32();
		if (m_sfnt != offset_table.sfnt_version)
		{
			LOG("The sfnt version of this font file is unsupported");
			return false;
		}

		offset_table.num_tables =		m_reader.get_uint16();
		offset_table.search_range =		m_reader.get_uint16();
		offset_table.entry_selector =	m_reader.get_uint16();
		offset_table.range_shift =		m_reader.get_uint16();

		m_table_records.reserve(offset_table.num_tables);

		for (int i = 0; i < offset_table.num_tables; i++)
		{
			if ((m_reader.get_array()[m_reader.get_position()] <= 0x7E     && m_reader.get_array()[m_reader.get_position()] >= 0x20) &&
				(m_reader.get_array()[m_reader.get_position() + 1] <= 0x7E && m_reader.get_array()[m_reader.get_position() + 1] >= 0x20) &&
				(m_reader.get_array()[m_reader.get_position() + 2] <= 0x7E && m_reader.get_array()[m_reader.get_position() + 2] >= 0x20) &&
				(m_reader.get_array()[m_reader.get_position() + 3] <= 0x7E && m_reader.get_array()[m_reader.get_position() + 3] >= 0x20))
			{
				tou::truetype::table_record record;
				size_t p = m_reader.get_position();

				std::string table_name = std::string() + m_reader.get_array()[p] + m_reader.get_array()[p + 1] + m_reader.get_array()[p + 2] + m_reader.get_array()[p + 3];
				record.tag =		m_reader.get_uint32();
				record.checksum =	m_reader.get_uint32();
				record.offset =		m_reader.get_uint32();
				record.length =		m_reader.get_uint32();
				
				m_table_records.insert({ table_name, record });
			}
			else
			{
				LOG("The table record contains an invalid tag or the number of tables recorded in the offset table is incorrect.");
				return false;
			}
		}

		// get number of glyphs in font file
		m_reader.set_position(m_table_records["maxp"].offset);
		tou::truetype::maxp maxp;
		if (m_reader.get_uint32() == 0x00010000)
		{
			m_num_glyphs = m_reader.get_uint16();
			//24
			m_reader.increment_position(24);
			maxp.max_component_depth = m_reader.get_uint16();
		}
		else
		{
			LOG("maxp version unsupported or not defined");
			return false;
		}

		// get number of horizontal metrics, neccessary for parsing hmtx table
		m_reader.set_position(m_table_records["hhea"].offset);
		m_reader.increment_position(34);
		uint16_t num_hori_metrics = m_reader.get_uint16();

		// get units per em and the index to location format from the head table
		m_reader.set_position(m_table_records["head"].offset);
		m_reader.increment_position(18);
		m_units_per_em = m_reader.get_uint16();
		m_reader.increment_position(30);
		int16_t index_to_loc_format = m_reader.get_int16();

		// parse hmtx table
		m_reader.set_position(m_table_records["hmtx"].offset);
		if (m_num_glyphs < num_hori_metrics)
		{
			for (uint16_t i = 0; i < num_hori_metrics; i++)
			{
				uint16_t advance_width =	m_reader.get_uint16();
				int16_t lsb =				m_reader.get_int16();
				m_hmtx.hmetrics.push_back({ advance_width, lsb });
			}
		}
		else if (m_num_glyphs >= num_hori_metrics)
		{
			for (uint16_t i = 0; i < num_hori_metrics; i++)
			{
				uint16_t advance_width =	m_reader.get_uint16();
				int16_t lsb =				m_reader.get_int16();
				m_hmtx.hmetrics.push_back({ advance_width, lsb });
			}
			for (uint16_t i = 0; i < static_cast<uint16_t>(m_num_glyphs - num_hori_metrics); i++)
				m_hmtx.hmetrics.push_back({ m_hmtx.hmetrics[num_hori_metrics - 1 + i].advance_width, m_hmtx.hmetrics[num_hori_metrics - 1 + i].lsb });
		}

		// parse loca table
		m_reader.set_position(m_table_records["loca"].offset);
		if (index_to_loc_format == 0)
		{
			uint64_t n = ((uint64_t)m_num_glyphs) + 1;
			for (uint64_t i = 0; i < n; i++)
				m_loca_offset32.push_back(((uint32_t)m_reader.get_uint16()) * 2);
		}
		else if (index_to_loc_format == 1)
		{
			uint64_t n = ((uint64_t)m_num_glyphs) + 1;
			for (uint64_t i = 0; i < n; i++)
				m_loca_offset32.push_back(m_reader.get_uint32());
		}
		else
		{
			LOG("Invalid index to Location Format!");
			return false;
		}

		// parse cmap table
		m_reader.set_position(m_table_records["cmap"].offset);
		// first is cmap head
		m_cmap.first.version =		m_reader.get_uint16();
		m_cmap.first.num_tables =	m_reader.get_uint16();
		for (uint64_t i = 0; i < m_cmap.first.num_tables; i++)
		{
			tou::truetype::cmap_encoding encoding;
			encoding.platform_id =	m_reader.get_uint16();
			encoding.encoding_id =	m_reader.get_uint16();
			encoding.offset =		m_reader.get_uint32();
			m_cmap.first.encoding_records.push_back(encoding);
		}
		bool format4_exists = false;
		for (auto& encoding_record : m_cmap.first.encoding_records)
		{
			m_reader.set_position((uint64_t)m_table_records["cmap"].offset + (uint64_t)encoding_record.offset);
			if ((encoding_record.platform_id == 3) && (encoding_record.encoding_id == 1))
			{
				// Unicode BMP font with data stored using cmap subtable format 4
				format4_exists = true;
				tou::truetype::cmap_format4 format;
				//format.format =			m_reader.get_uint16();
				//format.length =			m_reader.get_uint16();
				//format.language =			m_reader.get_uint16();
				m_reader.increment_position(6);
				format.seg_count_x2 =	m_reader.get_uint16();
				//format.search_range =		m_reader.get_uint16();
				//format.entry_selector =	m_reader.get_uint16();
				//format.range_shift =		m_reader.get_uint16();
				m_reader.increment_position(6);

				m_seg_count = format.seg_count_x2 / 2;
				for (uint64_t i = 0; i < m_seg_count; i++)
					format.end_code.push_back(m_reader.get_uint16());

				//format.reserved_pad = m_reader.get_uint16();
				m_reader.increment_position(2);

				for (uint64_t i = 0; i < m_seg_count; i++)
					format.start_code.push_back(m_reader.get_uint16());

				for (uint64_t i = 0; i < m_seg_count; i++)
					format.id_delta.push_back(m_reader.get_int16());

				m_id_range_offset_from_filestart = m_reader.get_position();

				for (uint64_t i = 0; i < m_seg_count; i++)
					format.id_range_offset.push_back(m_reader.get_uint16());

				m_cmap.second = format;
			}
		}
		if (!format4_exists)
		{
			LOG("cmap subtable format 4 could not be found in the font file");
			return false;
		}
		return true;
	}

	uint16_t font_face::m_get_truetype_glyph_id(uint16_t unicode)
	{
		uint16_t glyph_id = 0;
		for (size_t i = 0; i < m_seg_count; i++)
		{
			if ((m_cmap.second.start_code[i] <= unicode) && (unicode <= m_cmap.second.end_code[i]))
			{
				if (m_cmap.second.id_range_offset[i] != 0)
				{
					size_t start_code_offset = (uint64_t)(unicode - m_cmap.second.start_code[i]) * 2;
					size_t current_range_offset = i * 2;
					size_t glyph_index_offset = m_id_range_offset_from_filestart + current_range_offset + m_cmap.second.id_range_offset[i] + start_code_offset;

					glyph_id = tou::join_bytes(m_reader.get_array()[glyph_index_offset], m_reader.get_array()[glyph_index_offset + 1]);

					if (glyph_id != 0)
						glyph_id = (glyph_id + m_cmap.second.id_delta[i]) & 0xffff;
				}
				else
					glyph_id = unicode + static_cast<uint16_t>(m_cmap.second.id_delta[i]);
			}
		}
		return glyph_id;
	}

	bool font_face::m_get_truetype_simple_glyph_header_data(font_face::truetype_glyph& glyph)
	{
		// function assumes glyph.id is valid or 0
		bool outline_present = false;
		if (static_cast<size_t>(glyph.id) + 1 >= m_loca_offset32.size())
			outline_present = (m_table_records["glyf"].length != m_loca_offset32[glyph.id]);
		else
			outline_present = (m_loca_offset32[glyph.id] != m_loca_offset32[static_cast<size_t>(glyph.id) + 1]);

		m_reader.set_position((uint64_t)m_table_records["glyf"].offset + (uint64_t)m_loca_offset32[glyph.id]);

		if (outline_present)
		{
			glyph.num_contours = m_reader.get_int16();
			glyph.x_min = m_reader.get_int16();
			glyph.y_min = m_reader.get_int16();
			glyph.x_max = m_reader.get_int16();
			glyph.y_max = m_reader.get_int16();
			glyph.advance_width = m_hmtx.hmetrics[glyph.id].advance_width;
			glyph.left_side_bearing = m_hmtx.hmetrics[glyph.id].lsb; // TODO: scenario where lsb is not in hMetrics
		}
		else
		{
			glyph.advance_width = m_hmtx.hmetrics[glyph.id].advance_width;
			return outline_present;
		}
		return outline_present;
	}

	void font_face::m_get_truetype_simple_glyph_data(font_face::truetype_glyph& glyph)
	{
		// function assumes glyph has valid header information and that m_reader is positioned correctly
		// simple glyph definition
		for (uint64_t j = 0; j < glyph.num_contours; j++)
			glyph.end_pts_of_contours.push_back(m_reader.get_uint16());

		std::vector<uint16_t>::iterator result = std::max_element(glyph.end_pts_of_contours.begin(), glyph.end_pts_of_contours.end());
		glyph.num_points = *result + 1;

		glyph.instruction_len = m_reader.get_uint16();
		if (glyph.instruction_len != 0)
		{
			for (uint64_t j = 0; j < glyph.instruction_len; j++)
				glyph.instructions.push_back(m_reader.get_uint8());
		}
		for (uint64_t b = 0; b < glyph.num_points; b++)
		{
			const uint8_t flag = m_reader.get_uint8();
			tou::truetype::glyph_flags flags_for_this_point;

			flags_for_this_point.on_curve_point = ((flag & ON_CURVE_POINT) == ON_CURVE_POINT);
			flags_for_this_point.x_short_vector = ((flag & X_SHORT_VECTOR) == X_SHORT_VECTOR);
			flags_for_this_point.y_short_vector = ((flag & Y_SHORT_VECTOR) == Y_SHORT_VECTOR);
			flags_for_this_point.x_is_same_or_positive_x_short_vector = ((flag & X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR) == X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR);
			flags_for_this_point.y_is_same_or_positive_y_short_vector = ((flag & Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR) == Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR);
			flags_for_this_point.overlap_simple = ((flag & OVERLAP_SIMPLE) == OVERLAP_SIMPLE);

			if ((flag & REPEAT_FLAG) == REPEAT_FLAG)
			{
				flags_for_this_point.repeat_flag = true;
				flags_for_this_point.repeat_count = m_reader.get_uint8();
				for (uint8_t v = 0; v < flags_for_this_point.repeat_count; v++)
				{
					glyph.flags_bool.push_back(flags_for_this_point);
					b++;
				}
				glyph.flags_bool.push_back(flags_for_this_point); // for the original flag
			}
			else
			{
				flags_for_this_point.repeat_count = 0;
				glyph.flags_bool.push_back(flags_for_this_point);
			}
		}
		// ready to parse coordinate arrays...
		for (size_t b = 0; b < glyph.num_points; b++)
		{
			if (glyph.flags_bool[b].x_short_vector)
			{
				// x coordinate is 1 byte
				if (glyph.flags_bool[b].x_is_same_or_positive_x_short_vector)
				{
					// x coordinate is positive
					int16_t x = (int16_t)m_reader.get_uint8();
					if (b > 0)
						x += glyph.x_coords[b - 1];
					glyph.x_coords.push_back(x);
				}
				else
				{
					// x coordinate is negative
					int16_t x = ((-1) * (int16_t)m_reader.get_uint8());
					if (b > 0)
						x += glyph.x_coords[b - 1];
					glyph.x_coords.push_back(x);
				}
			}
			else
			{
				// x coordinate is 2 bytes
				if (glyph.flags_bool[b].x_is_same_or_positive_x_short_vector)
				{
					// x coordinate is identical to previous x coordinate (or 0)
					if (b == 0)
						glyph.x_coords.push_back(0);
					else
						glyph.x_coords.push_back(glyph.x_coords[b - 1]);
				}
				else
				{
					// x coordinate is a signed 16-bit delta
					int16_t x = m_reader.get_int16();
					if (b > 0)
						x += glyph.x_coords[b - 1];
					glyph.x_coords.push_back(x);
				}
			}
		}
		for (size_t b = 0; b < glyph.num_points; b++)
		{
			if (glyph.flags_bool[b].y_short_vector)
			{
				// y coordinate is 1 byte
				if (glyph.flags_bool[b].y_is_same_or_positive_y_short_vector)
				{
					// y coordinate is positive
					int16_t y = (int16_t)m_reader.get_uint8();
					if (b > 0)
						y += glyph.y_coords[b - 1];
					glyph.y_coords.push_back(y);
				}
				else
				{
					// y coordinate is negative
					int16_t y = ((-1) * (int16_t)m_reader.get_uint8());
					if (b > 0)
						y += glyph.y_coords[b - 1];
					glyph.y_coords.push_back(y);
				}
			}
			else
			{
				// y coordinate is 2 bytes
				if (glyph.flags_bool[b].y_is_same_or_positive_y_short_vector)
				{
					// y coordinate is identical to previous y coordinate (or 0)
					if (b == 0)
						glyph.y_coords.push_back(0);
					else
						glyph.y_coords.push_back(glyph.y_coords[b - 1]);
				}
				else
				{
					// y coordinate is a signed 16-bit delta
					int16_t y = m_reader.get_int16();
					if (b > 0)
						y += glyph.y_coords[b - 1];
					glyph.y_coords.push_back(y);
				}
			}
		}
	}

	void font_face::m_get_truetype_component_glyph_data(std::vector<truetype::glyph_component>& components)
	{
		// function assumes glyph has valid header information and that m_reader is positioned correctly
		bool more_components = true;
		while (more_components)
		{
			truetype::glyph_component component;
			component.flag = m_reader.get_uint16();
			component.glyph_index = m_reader.get_uint16();

			// determine the data types of arg1 and arg2
			if (((component.flag & ARG_1_AND_2_ARE_WORDS) == ARG_1_AND_2_ARE_WORDS) && ((component.flag & ARGS_ARE_XY_VALUES) == ARGS_ARE_XY_VALUES))
			{
				// arguments are signed 16-bit xy values
				component.xy_arg1 = m_reader.get_int16();
				component.xy_arg2 = m_reader.get_int16();
			}
			else if (((component.flag & ARG_1_AND_2_ARE_WORDS) == ARG_1_AND_2_ARE_WORDS) && ((component.flag & ARGS_ARE_XY_VALUES) != ARGS_ARE_XY_VALUES))
			{
				// arguments are 16-bit unsigned point numbers
				component.pt_arg1 = m_reader.get_uint16();
				component.pt_arg2 = m_reader.get_uint16();
			}
			else if (((component.flag & ARG_1_AND_2_ARE_WORDS) != ARG_1_AND_2_ARE_WORDS) && ((component.flag & ARGS_ARE_XY_VALUES) == ARGS_ARE_XY_VALUES))
			{
				// arguments are signed 8-bit xy values
				component.xy_arg1 = static_cast<int16_t>(m_reader.get_array()[m_reader.get_position()]);
				component.xy_arg2 = static_cast<int16_t>(m_reader.get_array()[m_reader.get_position() + 1]);
				m_reader.increment_position(2);
			}
			else
			{
				// arguments are 8-bit unsigned point numbers
				component.pt_arg1 = static_cast<uint16_t>(m_reader.get_uint8());
				component.pt_arg2 = static_cast<uint16_t>(m_reader.get_uint8());
			}

			component.round_to_nearest_grid_line = ((component.flag & ROUND_XY_TO_GRID) == ROUND_XY_TO_GRID);

			if ((component.flag & WE_HAVE_A_SCALE) == WE_HAVE_A_SCALE)
			{
				// simple scale for the component
				component.scale = m_reader.get_int16();
			}
			else if ((component.flag & WE_HAVE_AN_X_AND_Y_SCALE) == WE_HAVE_AN_X_AND_Y_SCALE)
			{
				// the x direction will use a difference scale from the y direction
				component.x_scale = m_reader.get_int16();
				component.y_scale = m_reader.get_int16();
			}
			else if ((component.flag & WE_HAVE_A_TWO_BY_TWO) == WE_HAVE_A_TWO_BY_TWO)
			{
				// 2 by 2 transformation to scale this component
				component.x_scale = m_reader.get_int16();
				component.scale01 = m_reader.get_int16();
				component.scale10 = m_reader.get_int16();
				component.y_scale = m_reader.get_int16();
			}

			more_components = ((component.flag & MORE_COMPONENTS) == MORE_COMPONENTS);

			// after the last component, there are instructions to parse for this composite character
			component.instructions_present = ((component.flag & WE_HAVE_INSTRUCTIONS) == WE_HAVE_INSTRUCTIONS);

			// force the advanced width and lsb and rsb for the composite to be equal to those from this original glyph
			component.use_base_glyph_aw_and_lsb = ((component.flag & USE_MY_METRICS) == USE_MY_METRICS);
			component.overlap_compound = ((component.flag & OVERLAP_COMPOUND) == OVERLAP_COMPOUND); // https://docs.microsoft.com/en-us/typography/opentype/spec/glyf
			component.scaled_component_offset = ((component.flag & SCALED_COMPONENT_OFFSET) == SCALED_COMPONENT_OFFSET && (component.flag & UNSCALED_COMPONENT_OFFSET) != UNSCALED_COMPONENT_OFFSET);

			components.push_back(component);
		}

	}

	void font_face::m_get_truetype_glyph_data_by_id(font_face::truetype_glyph& glyph, uint16_t glyph_id)
	{
		// this overload function assumes we are getting a simple glyph
		if (glyph.id != glyph_id)
			glyph.id = glyph_id;

		if (!m_get_truetype_simple_glyph_header_data(glyph)) // responsible for positioning m_reader
			return;

		// simple glyph description
		m_get_truetype_simple_glyph_data(glyph);
	}

	void font_face::m_get_truetype_glyph_data_by_id(font_face::truetype_glyph& glyph, uint16_t glyph_id, std::vector<truetype::glyph_component>& components)
	{
		if (glyph.id != glyph_id)
			glyph.id = glyph_id;

		if (!m_get_truetype_simple_glyph_header_data(glyph)) // responsible for positioning m_reader
			return;

		if (glyph.num_contours >= 0)
		{
			// simple glyph description
			m_get_truetype_simple_glyph_data(glyph);
		}
		else if (glyph.num_contours < 0)
		{
			// composite glyph description
			m_get_truetype_component_glyph_data(components);
		}
	}

	font_face::truetype_glyph font_face::m_get_truetype_glyph(uint16_t unicode)
	{
		font_face::truetype_glyph glyph;
		std::vector<truetype::glyph_component> components;
		glyph.id = m_get_truetype_glyph_id(unicode);
		m_get_truetype_glyph_data_by_id(glyph, glyph.id, components);

		if (components.size() != 0)
		{
			glyph.num_contours = 0;
			// construct the composite glyph
			// this block calls m_get_glyph_data_by_id for all components
			// this block assumes m_reader is positioned directly after the last of component data (always the case if components.size() > 0)
			
			//std::vector<uint8_t> instructions;
			//if (components[components.size() - 1].instructions_present)
			//{
			//	uint16_t instruction_len = m_reader.get_uint16();
			//	if (instruction_len != 0)
			//	{
			//		for (uint16_t j = 0; j < instruction_len; j++)
			//			instructions.push_back(m_reader.get_uint8());
			//	}
			//}

			std::vector<truetype_glyph> glyph_pieces; // temp for debugging
			int glyph_index_for_base = 0;
			for (const auto& comp : components)
			{
				font_face::truetype_glyph glyph_piece;
				m_get_truetype_glyph_data_by_id(glyph_piece, comp.glyph_index);

				if (comp.use_base_glyph_aw_and_lsb)
					glyph_index_for_base = comp.glyph_index;
				
				if (comp.xy_arg1 || comp.xy_arg2)
				{
					// modify data for the piece if needed
					// xy_arg1 is x offset for points in piece, xy_arg2 is y offset
					// for pieces, we only care about point data and endpoint data

					for (int i = 0; i < glyph_piece.num_points; i++)
					{
						glyph_piece.x_coords[i] += comp.xy_arg1;
						glyph_piece.y_coords[i] += comp.xy_arg2;
					}

				}
				//else if (comp.pt_arg1 && comp.pt_arg2)
				//{
				//	// pt arg1 is original point to be matched to new composites entry as pt arg2
				//}
				for (int i = 0; i < glyph_piece.num_contours; i++)
				{
					glyph_piece.end_pts_of_contours[i] += glyph.num_points;
					glyph.end_pts_of_contours.push_back(glyph_piece.end_pts_of_contours[i]);
				}
				glyph.num_contours += glyph_piece.num_contours;
				glyph.num_points += glyph_piece.num_points;
				for (int i = 0; i < glyph_piece.num_points; i++)
				{
					glyph.flags_bool.push_back(glyph_piece.flags_bool[i]);
					glyph.x_coords.push_back(glyph_piece.x_coords[i]);
					glyph.y_coords.push_back(glyph_piece.y_coords[i]);
				}
				
				glyph_pieces.push_back(glyph_piece); // temp
			}

			if (glyph_index_for_base != 0)
			{
				glyph.advance_width = m_hmtx.hmetrics[glyph_index_for_base].advance_width;
				glyph.left_side_bearing = m_hmtx.hmetrics[glyph_index_for_base].lsb; // TODO: scenario where lsb is not in hMetrics
			}
			
		}

		return glyph;
	}

	font_face::bitmap_glyph font_face::m_rasterize_truetype_glyph(const font_face::truetype_glyph& g, float pointsize, bool render_outline, bool render_inside)
	{
		float dpi = 300.0f;
		font_face::truetype_glyph glyf = g;
		font_face::bitmap_glyph glyph;
		glyph.id = glyf.id;
		
		// we don't like negative values
		if (glyf.x_min < 0)
		{
			for (auto& x : glyf.x_coords)
				x += (-1 * glyf.x_min);

			glyf.x_max += (-1 * glyf.x_min);
			glyf.x_min += (-1 * glyf.x_min);
		}
		if (glyf.y_min < 0)
		{
			for (auto& y : glyf.y_coords)
				y += (-1 * glyf.y_min);

			glyf.y_max += (-1 * glyf.y_min);
			glyf.y_min += (-1 * glyf.y_min);
		}

		glyph.advance_x = roundf26(convert_to_f26(convert_to_pixel(static_cast<float>(glyf.advance_width), pointsize, dpi, static_cast<float>(m_units_per_em)))) / 64;

		// convert x_min, x_max, y_min, y_max to pixel values then convert and grid-fit the bounding box
		tou::glyph_bounding_box box;
		box.x_min = tou::floorf26(tou::convert_to_f26(tou::convert_to_pixel(static_cast<float>(glyf.x_min), pointsize, dpi, static_cast<float>(m_units_per_em))));
		box.x_max = tou::ceilf26( tou::convert_to_f26(tou::convert_to_pixel(static_cast<float>(glyf.x_max), pointsize, dpi, static_cast<float>(m_units_per_em))));
		box.y_min = tou::floorf26(tou::convert_to_f26(tou::convert_to_pixel(static_cast<float>(glyf.y_min), pointsize, dpi, static_cast<float>(m_units_per_em))));
		box.y_max = tou::ceilf26( tou::convert_to_f26(tou::convert_to_pixel(static_cast<float>(glyf.y_max), pointsize, dpi, static_cast<float>(m_units_per_em))));

		// get pixel dimensions of bitmap
		uint32_t width = ((box.x_max) / 64) + (((box.x_min / 64)) + 1);
		uint32_t height = ((box.y_max) / 64) + (((box.y_min / 64)) + 1);
		glyph.image.resize(width, height);

		// convert coordinate/outline coordinate values to fixed float (f26) values and place them in a map from least to greatest
		std::vector<tou::glyph_outline_segment> outline;
		std::map<uint32_t, std::vector<uint32_t>> outline_map;
		size_t coord_array_position = 0;
		for (int16_t i = 0; i < glyf.num_contours; i++) // we assume numContours is not negative
		{
			size_t endpoint = glyf.end_pts_of_contours[i];
			bool phantom_point = false;
			tou::ivec2 phantom_point_value = { 0, 0 };
			for (size_t j = coord_array_position; j <= endpoint; j++)
			{
				if (j != endpoint)
				{
					if (glyf.flags_bool[j].on_curve_point && glyf.flags_bool[j + 1].on_curve_point)
					{
						// linear description
						tou::glyph_outline_segment seg = tou::calculate_segment_path(pointsize, dpi, FLT(m_units_per_em),
							FLT(glyf.x_coords[j]), FLT(glyf.x_coords[j + 1]), FLT(glyf.y_coords[j]), FLT(glyf.y_coords[j + 1]), false, 0.0f, 0.0f);

						outline.push_back(seg);
						add_to_map(seg, outline_map);
					}
					else if (glyf.flags_bool[j].on_curve_point && !glyf.flags_bool[j + 1].on_curve_point)
					{
						size_t array_pos_of_p2 = j + 2;
						if (array_pos_of_p2 > endpoint) // we've reached the end of this section of the glyph, make sure to set coord_array_position appropriately
							array_pos_of_p2 = coord_array_position;

						tou::ivec2 p2 = { static_cast<uint32_t>(glyf.x_coords[array_pos_of_p2]), static_cast<uint32_t>(glyf.y_coords[array_pos_of_p2]) };
						if (!glyf.flags_bool[array_pos_of_p2].on_curve_point)
						{
							// find phantom point and set it equal to p2
							phantom_point = true;
							p2 = tou::midpoint(glyf.x_coords[j + 1], glyf.x_coords[array_pos_of_p2], glyf.y_coords[j + 1], glyf.y_coords[array_pos_of_p2]);
							phantom_point_value = p2; //p2 needs to be held somewhere to be used as p1 of next control point
						}

						// bezier description
						auto segment = tou::calculate_segment_path(pointsize, dpi, FLT(m_units_per_em),
							FLT(glyf.x_coords[j]), FLT(p2.x), FLT(glyf.y_coords[j]), FLT(p2.y), true, FLT(glyf.x_coords[j + 1]), FLT(glyf.y_coords[j + 1]));

						outline.push_back(segment);
						add_to_map(segment, outline_map);
						
						j++; // 'jump to p2' (this will terminate the for loop for edge case) (the j++ in the for statement completes our travel to p2)

						if (j + 1 > endpoint)
							coord_array_position = j + 1;
					}
					else if (!glyf.flags_bool[j].on_curve_point && glyf.flags_bool[j + 1].on_curve_point && phantom_point)
					{
						// this happens because we previously dealt with a phantom point
						// p1 = phantom point, control point = j, p2 = j + 1
						phantom_point = false;
						tou::ivec2 p1 = phantom_point_value;

						// bezier description
						auto segment = tou::calculate_segment_path(pointsize, dpi, FLT(m_units_per_em),
							FLT(p1.x), FLT(glyf.x_coords[j + 1]), FLT(p1.y), FLT(glyf.y_coords[j + 1]), true, FLT(glyf.x_coords[j]), FLT(glyf.y_coords[j]));

						outline.push_back(segment);
						add_to_map(segment, outline_map);
					}
					else if (!glyf.flags_bool[j].on_curve_point && !glyf.flags_bool[j + 1].on_curve_point && phantom_point)
					{
						// more than one phantom point occured
						tou::ivec2 p1 = phantom_point_value;

						size_t array_pos_of_unrelated_control_point = j + 1;

						if (array_pos_of_unrelated_control_point > endpoint) // we've reached the end of this section of the glyph, make sure to set coord_array_position appropriately
							array_pos_of_unrelated_control_point = coord_array_position;

						tou::ivec2 p2 = tou::midpoint(glyf.x_coords[j], glyf.x_coords[array_pos_of_unrelated_control_point], glyf.y_coords[j], glyf.y_coords[array_pos_of_unrelated_control_point]);
						
						phantom_point_value = p2; //p2 needs to be held somewhere to be used as p1 of next control point

						// bezier description
						auto segment = tou::calculate_segment_path(pointsize, dpi, FLT(m_units_per_em),
							FLT(p1.x), FLT(p2.x), FLT(p1.y), FLT(p2.y), true, FLT(glyf.x_coords[j]), FLT(glyf.y_coords[j]));

						outline.push_back(segment);
						add_to_map(segment, outline_map);
					}
				}
				else
				{
					if (glyf.flags_bool[j].on_curve_point && glyf.flags_bool[coord_array_position].on_curve_point)
					{
						tou::glyph_outline_segment segment = tou::calculate_segment_path(pointsize, dpi, FLT(m_units_per_em),
							FLT(glyf.x_coords[j]), FLT(glyf.x_coords[coord_array_position]), FLT(glyf.y_coords[j]), FLT(glyf.y_coords[coord_array_position]),
							false, 0.0f, 0.0f);

						outline.push_back(segment);
						add_to_map(segment, outline_map);
					}
					else if (!glyf.flags_bool[j].on_curve_point && glyf.flags_bool[coord_array_position].on_curve_point && phantom_point)
					{
						phantom_point = false;

						// use phantom point as p1, j as control, and coord_array_position as p2
						auto segment = tou::calculate_segment_path(pointsize, dpi, FLT(m_units_per_em),
							FLT(phantom_point_value.x), FLT(glyf.x_coords[coord_array_position]),
							FLT(phantom_point_value.y), FLT(glyf.y_coords[coord_array_position]), true, FLT(glyf.x_coords[j]), FLT(glyf.y_coords[j]));

						outline.push_back(segment);
						add_to_map(segment, outline_map);
					}
					coord_array_position = j + 1;
				}

			}
		}
		if (render_outline)
		{
			for (const auto& segment : outline)
			{
				uint32_t pxstart = segment.start.x / 64;
				uint32_t pystart = segment.start.y / 64;
				uint32_t pxend = segment.end.x / 64;
				uint32_t pyend = segment.end.y / 64;
				glyph.image[{pxstart, pystart}] = FILL_BLK;
				glyph.image[{pxend, pyend}] = FILL_BLK;
				for (const auto& pair : segment.values)
				{
					uint32_t py = pair.first / 64;
					for (size_t i = 0; i < pair.second.size(); i++)
					{
						uint32_t px = pair.second[i] / 64;
						glyph.image[{px, py}] = FILL_BLK;
					}
				}
			}
		}
		if (render_inside)
		{
			for (auto& pair : outline_map)
			{
				uint32_t y = pair.first;
				uint32_t py = y / 64;
				if (pair.second.size() != 1)
				{



					for (size_t i = 0; i < pair.second.size(); i++)
					{
						if (i == pair.second.size() - 1) break;
						uint32_t xstart = pair.second[i];
						uint32_t xend = pair.second[i + 1];
						if (interior_row(outline, y, xstart, xend))
						{
							for (uint32_t x = xstart; x <= xend; x += 64)
								glyph.image[{(x / 64), py}] = FILL_BLK;
						}
					}
				}
			}
		}
		return glyph;
	}
}
