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

	tou::fvec2 calculate_vectorial_point_at_t(float t, float x1, float x2, float y1, float y2, bool bezier, float cx, float cy)
	{
		tou::fvec2 out{ 0, 0 };
		if (bezier)
		{
			out.x = tou::bezier2(t, x1, x2, cx);
			out.y = tou::bezier2(t, y1, y2, cy);
		}
		else
		{
			out.x = tou::linear(t, x1, x2);
			out.y = tou::linear(t, y1, y2);
		}
		return out;
	}

	//template<class InputIt>
	//constexpr InputIt find_glyph_value(InputIt first, InputIt last, const glyph_value& value)
	//{
	//	for (; first != last; ++first)
	//	{
	//		if (*first == value.f26)
	//			return first;
	//	}
	//	return last;
	//}

	std::vector<glyph_value>::iterator find_glyph_value(std::vector<glyph_value>::iterator first, std::vector<glyph_value>::iterator last, uint32_t value)
	{
		for (; first != last; ++first)
		{
			if (first->f26 == value)
				return first;
		}
		return last;
	}

	//#define FINDV(v, n) std::find(v.begin(), v.end(), n) != v.end()
	#define glyph_value_exists(v, x) (find_glyph_value(v.begin(), v.end(), x) != v.end())

	void calculate_over_t(tou::glyph_outline_segment& seg, float t_increment, float x1, float x2, float y1, float y2, bool bezier, float cx, float cy)
	{
		tou::ivec2 previous_point;
		for (float t = 0.00f; t <= 1.00f; t += t_increment)
		{
			tou::ivec2 pt = tou::calculate_point_at_t(t, FLT(seg.start.x), FLT(seg.end.x), FLT(seg.start.y), FLT(seg.end.y), bezier, FLT(seg.control.x), FLT(seg.control.y));
			if (previous_point != pt)
			{
				if (!glyph_value_exists(seg.values[pt.y], pt.x))
				{
					tou::fvec2 vectorial_point = tou::calculate_vectorial_point_at_t(t, x1, x2, y1, y2, bezier, cx, cy);
					glyph_value y;
					glyph_value x;
					y.f26 = pt.y;
					y.vectorial = vectorial_point.y;
					x.f26 = pt.x;
					x.vectorial = vectorial_point.x;
					seg.values[y.f26].push_back(x);
					previous_point = pt;
				}
			}
		}
	}

	void calculate_segment_path(tou::glyph_outline_segment& seg, float pointsize, float dpi, float units_per_em, float x1, float x2, float y1, float y2, bool bezier, float cx, float cy)
	{
		//tou::glyph_outline_segment seg;
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
		if (bezier && (difference == 0))
			difference = static_cast<int>(y2) - static_cast<int>(y1);
		seg.denotes_on_transition = (difference > 0);
		seg.horizontal = ((difference == 0) && !bezier);
		seg.bezier = bezier;

		// end.x - start.x -> positive means we went left to right, negative means we went right to left
		int x_difference = static_cast<int>(x2) - static_cast<int>(x1);
		if (x_difference != 0)
			seg.direction = (x_difference > 0) ? 1 : 0;
		seg.vertical = (!seg.horizontal && !seg.bezier && x_difference == 0);

		if (pointsize < 32.0f)
		{
			// t += 0.02f
			calculate_over_t(seg, 0.02f, x1, x2, y1, y2, bezier, cx, cy);
		}
		else
		{
			// t += 0.001f
			calculate_over_t(seg, 0.001f, x1, x2, y1, y2, bezier, cx, cy);
		}
		//return seg;
	}

	//void add_to_map(glyph_outline_segment& segment, std::map<uint32_t, std::vector<uint32_t>>& outline_map)
	//{
	//	for (auto& pair : segment.values)
	//	{
	//		for (int p = 0; p < pair.second.size(); p++)
	//		{
	//			if (!(FIND(outline_map, pair.first, pair.second[p])))
	//				outline_map[pair.first].push_back(pair.second[p]);
	//		}
	//		std::sort(outline_map[pair.first].begin(), outline_map[pair.first].end());
	//	}
	//}

	//bool interior_row(std::vector<tou::glyph_outline_segment>& outline, uint32_t y, uint32_t xstart, uint32_t xend)
	//{
	//	// check to see if the given xstart belongs to an 'on' segment and if the given xend belongs to an 'off' segment
	//	// function assumes y is valid and xstart and xend exist in the outline
	//	bool valid_start = false, valid_end = false;
	//	for (auto& seg : outline)
	//	{
	//		if (seg.values.find(y) != seg.values.end())
	//		{
	//			if ((FIND(seg.values, y, xstart)))
	//			{
	//				if ((seg.denotes_on_transition) && (!seg.horizontal))
	//				{
	//					valid_start = true;
	//				}
	//				else if ((!seg.denotes_on_transition) && (!seg.horizontal) && seg.bezier)
	//				{
	//					if ((xstart >= seg.control.x) && (y < seg.end.y))
	//						valid_start = true;
	//				}
	//			}
	//			if (((FIND(seg.values, y, xend))))
	//			{
	//				if ((!seg.denotes_on_transition) && (!seg.horizontal))
	//					valid_end = true;
	//			}
	//		}
	//
	//		if (valid_start && valid_end)
	//			break;
	//	}
	//	return (valid_start && valid_end);
	//}

	//bool determine_on_segment_trait_for_horizontal_segment_value(const glyph_outline_value& x, const std::vector<glyph_outline_segment>& segmented_outline)
	//{
	//	// to change:
	//	// take in pair.second
	//	// identify duplicates for given x
	//	// refer to duplicate's segment as the 'adjacent' segment
	//	// check for neighboring horizontals by doing what this function does but
	//	// in the 'direction' of the adjacent segment (if adj has higher segment id, increase through segment vector/decrease if adj has lower segment id)
	//	// must be careful that the given x value is confirmed to be a member of a horizontal segment, meaning every value with
	//	// a differing segment id can be safely assumed to be adjacents of some kind
	//
	//	bool segment_is_horizontal = true;
	//	int segment_id = x.associated_segment_id - 1;
	//	while (segment_is_horizontal)
	//	{
	//		if (segment_id < 0) break;
	//		segment_is_horizontal = segmented_outline[segment_id].horizontal;
	//		segment_id--;
	//	}
	//	//x.member_of_on_segment = segmented_outline[segment_id + 1].denotes_on_transition;
	//	return segmented_outline[static_cast<size_t>(segment_id) + 1].denotes_on_transition;
	//}

	int get_adjacent_segment_id(const glyph_outline_value& x, uint32_t y/*, const std::vector<glyph_outline_value>& values*/, const std::vector<glyph_outline_segment>& segmented_outline)
	{
		// this function helps us figure how to treat x when it's an endpoint in a horizontal segment
		// we want to treat the on/off quality of x the same as the segment adjacent to it
		//if (!x.part_of_horizontal) return -1;
		//tou::ivec2 start = segmented_outline[x.associated_segment_id].start;
		//tou::ivec2 end = segmented_outline[x.associated_segment_id].end;
		//int difference = start.x - end.x;
		//if (difference > 0)
		//{
		// direction of calculation is right to left
		// this horizontal's 'end' is the left adjacent segment's start (potentially with segment id 1 greater than horizontal)
		// this horizontal's 'start' is the right adjacentment segment's end (potentially with segment id 1 less than horizontal)

		// first check for simplest case (to make this more accurate, take in y as argument)
		if ( (x.associated_segment_id + 1 < static_cast<int>(segmented_outline.size())) && (x.associated_segment_id - 1 > 0) )
		{
			if (segmented_outline[static_cast<size_t>(x.associated_segment_id) + 1].start.x == x.value.f26 && segmented_outline[static_cast<size_t>(x.associated_segment_id) + 1].start.y == y)
			{
				// looking at left adjacent segment
				return x.associated_segment_id + 1;
			}
			else if (segmented_outline[static_cast<size_t>(x.associated_segment_id) - 1].end.x == x.value.f26 && segmented_outline[static_cast<size_t>(x.associated_segment_id) - 1].end.y == y)
			{
				// looking at right adjacent segment
				return x.associated_segment_id - 1;
			}
		}

		// check case where the adjacent segment doesn't have a segment id +-1 from x.associated_segment_id
		for (size_t i = 0; i < segmented_outline.size(); i++)
		{
			if (segmented_outline[i].start.x == x.value.f26 && segmented_outline[i].start.y == y && x.associated_segment_id != i)
			{
				// looking at left adjacent segment
				return static_cast<int>(i);
			}
			else if (segmented_outline[i].end.x == x.value.f26 && segmented_outline[i].end.y == y && x.associated_segment_id != i)
			{
				// looking at right adjacent segment
				return static_cast<int>(i);
			}
		}

		return -1; // no adjacent segment found
	}

	std::pair<bool, glyph_outline_value> check_for_duplicate_value_with_differing_trait(const glyph_outline_value& x, uint32_t y, const std::vector<glyph_outline_value>& values, const std::vector<glyph_outline_segment>& segmented_outline)
	{
		// function returns true if a duplicate value with a differing segment trait is found
		// function returns the identified value, if there is one

		for (size_t j = 0; j < values.size(); j++)
		{
			//if ((values[j].value == x.value) && (values[j].member_of_on_segment != x.member_of_on_segment))
			//	return { true, values[j] };

			// since segments are computed in order from least to greatest, the first instance of a lower segment id shall be taken
			// this lets us get the segment with the lowest ranking id every time
			//if ((values[j].value == x.value) && values[j].associated_segment_id < x.associated_segment_id)
			//	return { true, values[j] };

			if ((values[j].value.f26 == x.value.f26) && values[j].associated_segment_id != x.associated_segment_id)
			{
				return { true, values[j] };

				// note to self: need to pay attention to direction of calculation to determine which segment id to prefer
				// direction of left to right -> prefer right adjacent segment
				// direction of right to left -> prefer left adjacent segment
				
				//if (values[j].associated_segment_id < x.associated_segment_id)
				//	return { true, values[j] };
				//else
				//	return { true, x };

				//int segment_id = get_adjacent_segment_id(x, y, segmented_outline);
				//if (segment_id != -1 && values[j].associated_segment_id == segment_id)
				//	return { true, values[j] };
			}

		}
		return { false, glyph_outline_value() };
	}

	bool check_for_and_fill_interior_range(uint32_t y,  const glyph_outline_value& xstart, const glyph_outline_value& xend, const std::vector<glyph_outline_segment>& segments, tou::bitmap_image& glyph_bitmap)
	{
		if (xstart.value.f26 == xend.value.f26) return false;
		bool fill_this_range = (xstart.member_of_on_segment) && (!xend.member_of_on_segment);
		if (!fill_this_range && ((xstart.associated_segment_id == xend.associated_segment_id) ||
			((xstart.associated_segment_id - xend.associated_segment_id == 1) || (xstart.associated_segment_id - xend.associated_segment_id == -1))))
		{
			// if these two x values are not neighbors, assume it is part of a greater interior range and fill inbetween them regardless
			// there is no way two x values belonging to the same segment (or directly neighboring segment) could have different 
			// on and off segment traits
			fill_this_range = ((static_cast<int>(xend.value.f26) - static_cast<int>(xstart.value.f26)) > 64 &&
				(xstart.member_of_on_segment && xend.member_of_on_segment));
		}
		if (!fill_this_range)
		{
			if (!xstart.member_of_on_segment && xstart.part_of_bezier)
			{
				//if ((xstart.value >= xstart.associated_segment_control.x) && (y < xstart.associated_segment_end.y))
				if ((xstart.value.f26 >= segments[xstart.associated_segment_id].control.x) && (y < segments[xstart.associated_segment_id].end.y))
					fill_this_range = true;
			}
		}
		if (fill_this_range)
		{
			for (uint32_t x = xstart.value.f26; x <= xend.value.f26; x += 64)
			{
				glyph_bitmap[{(x / 64), (y / 64)}] = FILL_BLK;
			}
		}
		return fill_this_range;
	}

	//std::vector<size_t> get_segment_index(std::vector<glyph_outline_segment>& outline, uint32_t y, uint32_t x)
	//{
	//	std::vector<size_t> indexes;
	//	for (size_t i = 0; i < outline.size(); i++)
	//	{
	//		if (outline[i].values.find(y) != outline[i].values.end())
	//			if (FIND(outline[i].values, y, x))
	//				indexes.push_back(i);
	//	}
	//	return indexes;
	//}

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
		// we assume a ttf file has been provided for parsing
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

		if (g.id == 0) LOG("An empty glyph was returned as a bitmap");

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
				uint64_t p = m_reader.get_position();

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
				m_hmtx.hmetrics.push_back({ m_hmtx.hmetrics[static_cast<size_t>(num_hori_metrics) - 1 + i].advance_width, m_hmtx.hmetrics[static_cast<size_t>(num_hori_metrics) - 1 + i].lsb });
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
		for (uint64_t i = 0; i < m_seg_count; i++)
		{
			if ((m_cmap.second.start_code[i] <= unicode) && (unicode <= m_cmap.second.end_code[i]))
			{
				if (m_cmap.second.id_range_offset[i] != 0)
				{
					uint64_t start_code_offset = (uint64_t)(unicode - m_cmap.second.start_code[i]) * 2;
					uint64_t current_range_offset = i * 2;
					uint64_t glyph_index_offset = m_id_range_offset_from_filestart + current_range_offset + m_cmap.second.id_range_offset[i] + start_code_offset;

					glyph_id = tou::join_bytes(m_reader.get_array()[glyph_index_offset], m_reader.get_array()[glyph_index_offset + 1]);

					if (glyph_id != 0)
						glyph_id = (glyph_id + m_cmap.second.id_delta[i]) & 0xffff;
				}
				else
					glyph_id = unicode + m_cmap.second.id_delta[i];
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
		for (uint64_t b = 0; b < glyph.num_points; b++)
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
		for (uint64_t b = 0; b < glyph.num_points; b++)
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

			//std::vector<truetype_glyph> glyph_pieces; // temp for debugging
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
				
				//glyph_pieces.push_back(glyph_piece); // temp
			}

			if (glyph_index_for_base != 0)
			{
				glyph.advance_width = m_hmtx.hmetrics[glyph_index_for_base].advance_width;
				glyph.left_side_bearing = m_hmtx.hmetrics[glyph_index_for_base].lsb; // TODO: scenario where lsb is not in hMetrics
			}
			
		}

		return glyph;
	}

	bool compare_glyph_outline_values(const glyph_outline_value& a, const glyph_outline_value& b)
	{
		return (a.value.vectorial < b.value.vectorial);
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

		std::vector<tou::glyph_outline_segment> segmented_outline;
		size_t coord_array_position = 0;

		for (int16_t i = 0; i < glyf.num_contours; i++) // we assume num_contours is not negative
		{
			int endpoint = glyf.end_pts_of_contours[i];
			bool phantom_point = false;
			tou::ivec2 phantom_point_value = { 0, 0 };

			for (size_t j = coord_array_position; j <= endpoint; j++)
			{
				if (j != endpoint)
				{
					if (glyf.flags_bool[j].on_curve_point && glyf.flags_bool[j + 1].on_curve_point)
					{
						//////////////////////////////////////////////////////////////////////////////////////////////
						//////////////////////////////////////////////////////////////////////////////////////////////
						//////////////////////////////////////////////////////////////////////////////////////////////
						tou::glyph_outline_segment seg;
						tou::calculate_segment_path(seg, pointsize, dpi, FLT(m_units_per_em), FLT(glyf.x_coords[j]), FLT(glyf.x_coords[j + 1]), FLT(glyf.y_coords[j]), FLT(glyf.y_coords[j + 1]), false, 0.0f, 0.0f);
						segmented_outline.push_back(seg);

					}
					else if (glyf.flags_bool[j].on_curve_point && !glyf.flags_bool[j + 1].on_curve_point)
					{
						//////////////////////////////////////////////////////////////////////////////////////////////
						//////////////////////////////////////////////////////////////////////////////////////////////
						//////////////////////////////////////////////////////////////////////////////////////////////
						size_t array_pos_of_p2 = j + 2;
						if (array_pos_of_p2 > endpoint) array_pos_of_p2 = coord_array_position; // we've reached the end of this section of the glyph, make sure to set coord_array_position appropriately

						tou::ivec2 p2 = { static_cast<uint32_t>(glyf.x_coords[array_pos_of_p2]), static_cast<uint32_t>(glyf.y_coords[array_pos_of_p2]) };

						if (!glyf.flags_bool[array_pos_of_p2].on_curve_point)
						{
							// find phantom point and set it equal to p2
							phantom_point = true;
							p2 = tou::midpoint(glyf.x_coords[j + 1], glyf.x_coords[array_pos_of_p2], glyf.y_coords[j + 1], glyf.y_coords[array_pos_of_p2]);
							phantom_point_value = p2; //p2 needs to be held somewhere to be used as p1 of next control point
						}

						tou::glyph_outline_segment seg;
						tou::calculate_segment_path(seg, pointsize, dpi, FLT(m_units_per_em), FLT(glyf.x_coords[j]), FLT(p2.x), FLT(glyf.y_coords[j]), FLT(p2.y), true, FLT(glyf.x_coords[j + 1]), FLT(glyf.y_coords[j + 1]));
						segmented_outline.push_back(seg);
						
						j++; // 'jump to p2' (this will terminate the for loop for edge case) (the j++ in the for statement completes our travel to p2)
						if (j + 1 > endpoint) coord_array_position = j + 1;

					}
					else if (!glyf.flags_bool[j].on_curve_point && glyf.flags_bool[j + 1].on_curve_point && phantom_point)
					{
						//////////////////////////////////////////////////////////////////////////////////////////////
						//////////////////////////////////////////////////////////////////////////////////////////////
						//////////////////////////////////////////////////////////////////////////////////////////////
						// this happens because we previously dealt with a phantom point
						// p1 = phantom point, control point = j, p2 = j + 1

						phantom_point = false;
						tou::ivec2 p1 = phantom_point_value;

						tou::glyph_outline_segment seg;
						tou::calculate_segment_path(seg, pointsize, dpi, FLT(m_units_per_em), FLT(p1.x), FLT(glyf.x_coords[j + 1]), FLT(p1.y), FLT(glyf.y_coords[j + 1]), true, FLT(glyf.x_coords[j]), FLT(glyf.y_coords[j]));
						segmented_outline.push_back(seg);

					}
					else if (!glyf.flags_bool[j].on_curve_point && !glyf.flags_bool[j + 1].on_curve_point && phantom_point)
					{
						//////////////////////////////////////////////////////////////////////////////////////////////
						//////////////////////////////////////////////////////////////////////////////////////////////
						//////////////////////////////////////////////////////////////////////////////////////////////
						// more than one phantom point occurred
						tou::ivec2 p1 = phantom_point_value;
						size_t array_pos_of_unrelated_control_point = j + 1;
						if (array_pos_of_unrelated_control_point > endpoint) array_pos_of_unrelated_control_point = coord_array_position; // we've reached the end of a section of the glyph, make sure to set coord_array_position appropriately

						tou::ivec2 p2 = tou::midpoint(glyf.x_coords[j], glyf.x_coords[array_pos_of_unrelated_control_point], glyf.y_coords[j], glyf.y_coords[array_pos_of_unrelated_control_point]);
						phantom_point_value = p2; //p2 needs to be held somewhere to be used as p1 of next control point

						tou::glyph_outline_segment seg;
						tou::calculate_segment_path(seg, pointsize, dpi, FLT(m_units_per_em), FLT(p1.x), FLT(p2.x), FLT(p1.y), FLT(p2.y), true, FLT(glyf.x_coords[j]), FLT(glyf.y_coords[j]));
						segmented_outline.push_back(seg);

					}
				}
				else
				{
					if (glyf.flags_bool[j].on_curve_point && glyf.flags_bool[coord_array_position].on_curve_point)
					{
						//////////////////////////////////////////////////////////////////////////////////////////////
						//////////////////////////////////////////////////////////////////////////////////////////////
						//////////////////////////////////////////////////////////////////////////////////////////////
						tou::glyph_outline_segment seg;
						tou::calculate_segment_path(seg, pointsize, dpi, FLT(m_units_per_em), FLT(glyf.x_coords[j]), FLT(glyf.x_coords[coord_array_position]), FLT(glyf.y_coords[j]), FLT(glyf.y_coords[coord_array_position]), false, 0.0f, 0.0f);
						segmented_outline.push_back(seg);

					}
					else if (!glyf.flags_bool[j].on_curve_point && glyf.flags_bool[coord_array_position].on_curve_point && phantom_point)
					{
						//////////////////////////////////////////////////////////////////////////////////////////////
						//////////////////////////////////////////////////////////////////////////////////////////////
						//////////////////////////////////////////////////////////////////////////////////////////////
						// use phantom point as p1, j as control, and coord_array_position as p2
						phantom_point = false;
						tou::glyph_outline_segment seg;
						tou::calculate_segment_path(seg, pointsize, dpi, FLT(m_units_per_em), FLT(phantom_point_value.x), FLT(glyf.x_coords[coord_array_position]), FLT(phantom_point_value.y), FLT(glyf.y_coords[coord_array_position]), true, FLT(glyf.x_coords[j]), FLT(glyf.y_coords[j]));
						segmented_outline.push_back(seg);
						
					}
					coord_array_position = j + 1;
				}
			}
		}

		// April 2022: Make map where each x value has information about its parent segment
		std::map<uint32_t, std::vector<tou::glyph_outline_value>> complete_outline;
		int segment_id = 0;
		for (const auto& segment : segmented_outline)
		{
			for (const auto& pair : segment.values)
			{
				uint32_t y = pair.first;
				for (glyph_value x_value : pair.second)
				{
					// note: there could be more than one x for a given y in a segment (horizontal and bezier cases)
					glyph_outline_value x;
					x.value = x_value;
					x.associated_segment_id = segment_id;
					x.member_of_on_segment = segment.denotes_on_transition;
					x.part_of_bezier = segment.bezier;
					x.part_of_horizontal = segment.horizontal;
					complete_outline[y].push_back(x);
				}
				//std::sort(complete_outline[y].begin(), complete_outline[y].end(), compare_glyph_outline_values);
			}
			segment_id++;
		}
		// sort the x values in complete_outline (done by comparing their vectorial values rather than their f26 values)
		for (auto& pair : complete_outline)
		{
			std::sort(pair.second.begin(), pair.second.end(), compare_glyph_outline_values);
		}
		if (render_outline)
		{
			for (const auto& segment : segmented_outline)
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
					for (int i = 0; i < pair.second.size(); i++)
					{
						uint32_t px = pair.second[i].f26 / 64;
						glyph.image[{px, py}] = FILL_BLK;
					}
				}
			}
		}		
		if (render_inside)
		{
			for (const auto& pair : complete_outline)
			{
				uint32_t y = pair.first;
				uint32_t py = y / 64;
				if (pair.second.size() != 1)
				{
					glyph_outline_value prev_xstart;
					glyph_outline_value prev_xend;
					bool prev_range_was_filled = false;
					for (size_t i = 0; i < pair.second.size(); i++)
					{
						if (i == pair.second.size() - 1) break;

						glyph_outline_value xstart = pair.second[i];
						glyph_outline_value xend = pair.second[i + 1];

						// issue: unreliable as a neighboring segment may not have a segment id +-1 from current segment
						if ((prev_xend.value == xstart.value) && prev_range_was_filled)
						{
							if (!xend.member_of_on_segment && !prev_xend.member_of_on_segment && ((xend.associated_segment_id - prev_xend.associated_segment_id == -1) || xend.associated_segment_id - prev_xend.associated_segment_id == 1))
							{
								// treat current xstart as member of 'on' segment
								xstart.member_of_on_segment = true;
							}
						}

						prev_range_was_filled = check_for_and_fill_interior_range(y, xstart, xend, segmented_outline, glyph.image);
						if (prev_range_was_filled)
						{
							prev_xstart = xstart;
							prev_xend = xend;
							continue;
						}

						std::pair<bool, glyph_outline_value> xstart_res = check_for_duplicate_value_with_differing_trait(xstart, y, pair.second, segmented_outline);
						std::pair<bool, glyph_outline_value> xend_res = check_for_duplicate_value_with_differing_trait(xend, y, pair.second, segmented_outline);
						if (xstart_res.first && xend_res.first)
						{
							// get the adjacent segment and look at its traits
							if ((xstart.member_of_on_segment && !xstart_res.second.member_of_on_segment) || (!xstart.member_of_on_segment && xstart_res.second.member_of_on_segment))
							{
								if ((xend.member_of_on_segment && !xend_res.second.member_of_on_segment) || (!xend.member_of_on_segment && xend_res.second.member_of_on_segment))
								{
									// this range is indeterminate if the direction of calculation for both segments is right to left
									// range is filled otherwise
									int starting_id1 = xstart.associated_segment_id;
									int starting_id2 = xstart_res.second.associated_segment_id;
									int ending_id1 = xend.associated_segment_id;
									int ending_id2 = xend_res.second.associated_segment_id;
						
									if (!segmented_outline[starting_id1].vertical && !segmented_outline[starting_id2].vertical && !segmented_outline[ending_id1].vertical && !segmented_outline[ending_id2].vertical)
									{
										if (segmented_outline[starting_id1].direction == 0 && segmented_outline[starting_id2].direction == 0 &&
											segmented_outline[ending_id1].direction == 0 && segmented_outline[ending_id2].direction == 0)
										{
											prev_range_was_filled = false;
											prev_xstart = xstart;
											prev_xend = xend;
											continue;
										}
										else if (segmented_outline[starting_id1].direction == 1 && segmented_outline[starting_id2].direction == 1 &&
											segmented_outline[ending_id1].direction == 1 && segmented_outline[ending_id2].direction == 1)
										{
											xstart.member_of_on_segment = true;
											xend.member_of_on_segment = false;
											prev_range_was_filled = check_for_and_fill_interior_range(y, xstart, xend, segmented_outline, glyph.image);
											prev_xstart = xstart;
											prev_xend = xend;
											continue;
										}
									}
									else
									{
										// if we have a vertical segment, that segment must be an on segment, paired with an adjacent segment going from left to right
										// xstart should have a segment going left to right and an adjacent vertical 'on' segment
										// xend should have a segment going left to right and an adjacent vertical 'off' segment
										if (((segmented_outline[starting_id1].denotes_on_transition && segmented_outline[starting_id1].vertical && segmented_outline[starting_id2].direction == 1) || (segmented_outline[starting_id1].direction == 1 && segmented_outline[starting_id2].denotes_on_transition && segmented_outline[starting_id2].vertical)) &&
											((!segmented_outline[ending_id1].denotes_on_transition && segmented_outline[ending_id1].vertical && segmented_outline[ending_id2].direction == 1) || (segmented_outline[ending_id1].direction == 1 && !segmented_outline[ending_id2].denotes_on_transition && segmented_outline[ending_id2].vertical)))
										{
											xstart.member_of_on_segment = true;
											xend.member_of_on_segment = false;
											prev_range_was_filled = check_for_and_fill_interior_range(y, xstart, xend, segmented_outline, glyph.image);
											prev_xstart = xstart;
											prev_xend = xend;
											continue;
										}
										else if (((segmented_outline[starting_id1].denotes_on_transition && segmented_outline[starting_id1].vertical && segmented_outline[starting_id2].direction == 0) || (segmented_outline[starting_id1].direction == 0 && segmented_outline[starting_id2].denotes_on_transition && segmented_outline[starting_id2].vertical)) &&
											((!segmented_outline[ending_id1].denotes_on_transition && segmented_outline[ending_id1].vertical && segmented_outline[ending_id2].direction == 0) || (segmented_outline[ending_id1].direction == 0 && !segmented_outline[ending_id2].denotes_on_transition && segmented_outline[ending_id2].vertical)))
										{
											//xstart.member_of_on_segment = true;
											//xend.member_of_on_segment = false;
						
											// whichever has the lower segment id
											xstart = (xstart.associated_segment_id < xstart_res.second.associated_segment_id) ? (xstart) : (xstart_res.second);
						
											// whichever has the greater segment id
											xend = (xend.associated_segment_id > xend_res.second.associated_segment_id) ? (xend) : (xend_res.second);
						
											prev_range_was_filled = check_for_and_fill_interior_range(y, xstart, xend, segmented_outline, glyph.image);
											prev_xstart = xstart;
											prev_xend = xend;
											continue;
										}
										else
										{
											prev_range_was_filled = false;
											prev_xstart = xstart;
											prev_xend = xend;
											continue;
										}
									}
								}
							}
						
							if (!xstart.member_of_on_segment && xstart_res.second.member_of_on_segment)
								xstart = xstart_res.second;
						
							if (xend.member_of_on_segment && !xend_res.second.member_of_on_segment)
								xend = xend_res.second;
						
							if (xstart.part_of_horizontal)
							{
								int adjacent_segment_id = get_adjacent_segment_id(xstart, y, segmented_outline);
								if (adjacent_segment_id != -1)
									xstart.member_of_on_segment = segmented_outline[adjacent_segment_id].denotes_on_transition;
							}
							if (xend.part_of_horizontal)
							{
								int adjacent_segment_id = get_adjacent_segment_id(xend, y, segmented_outline);
								if (adjacent_segment_id != -1)
									xend.member_of_on_segment = segmented_outline[adjacent_segment_id].denotes_on_transition;
							}
						
							prev_range_was_filled = check_for_and_fill_interior_range(y, xstart, xend, segmented_outline, glyph.image);
							prev_xstart = xstart;
							prev_xend = xend;
							continue;
						}
						else
						{
							// issue: which value should be used should be dictated by which value occurs first vectorially
							// this should not be a point of contention if x values are already sorted by vectorial values
							// scrap this code
							// always deferring to the duplicate with the lower segment id may not be reliable
							if (xstart_res.first)
							{
								if (!xstart.member_of_on_segment && xstart_res.second.member_of_on_segment)
									xstart = xstart_res.second;
							}
						
							if (xend_res.first)
							{
								if (xend.member_of_on_segment && !xend_res.second.member_of_on_segment)
									xend = xend_res.second;
							}
						}
						
						// check if current values are part of a horizontal
						// issue: this should only be run if we know there is a duplicate
						if (xstart.part_of_horizontal)
						{
							int adjacent_segment_id = get_adjacent_segment_id(xstart, y, segmented_outline);
							if (adjacent_segment_id != -1)
								xstart.member_of_on_segment = segmented_outline[adjacent_segment_id].denotes_on_transition;
						}
						if (xend.part_of_horizontal)
						{
							int adjacent_segment_id = get_adjacent_segment_id(xend, y, segmented_outline);
							if (adjacent_segment_id != -1)
								xend.member_of_on_segment = segmented_outline[adjacent_segment_id].denotes_on_transition;
						}

						prev_range_was_filled = check_for_and_fill_interior_range(y, xstart, xend, segmented_outline, glyph.image);
						prev_xstart = xstart;
						prev_xend = xend;
					}
				}
			}
		}

		return glyph;
	}
}
