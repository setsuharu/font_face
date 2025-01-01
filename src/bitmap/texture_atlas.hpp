#pragma once

#include <map>
#include <unordered_map>

#include "bitmap.hpp"
#include "util.hpp"

// This class only exists to visualize the results of the glyph rasterizer at a greater scale

namespace tou
{
	struct texture_atlas_element
	{
		tou::fvec2 bottom_left{ 0.0f, 0.0f };
		tou::fvec2 top_right{ 0.0f, 0.0f };
	};

	template <typename id_type>
	class texture_atlas
	{
	public:
		texture_atlas() = default;
		~texture_atlas() = default;
		texture_atlas(uint32_t width, uint32_t height, const bitmap::argb32& background_color = { 0xFF, 0xFF, 0xFF, 0x00 });

		bool push(const tou::bitmap_image& bitmap, const id_type& id);
		tou::bitmap_image get();
		texture_atlas_element get_element(const id_type& id);

	private:
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_usedspace;
		bitmap::argb32 m_background_color;
		std::unordered_map<id_type, texture_atlas_element> m_ids;
		std::map<uint32_t, std::vector<std::pair<id_type, tou::bitmap_image>>> m_data; // key: bitmap w * h, value: vector of pairs <ids, bitmap>
	};

	template<typename id_type>
	inline texture_atlas<id_type>::texture_atlas(uint32_t width, uint32_t height, const bitmap::argb32& background_color)
		:m_width(width), m_height(height), m_usedspace(0), m_background_color(background_color)
	{
	}

	template<typename id_type>
	inline bool texture_atlas<id_type>::push(const tou::bitmap_image& bitmap, const id_type& id)
	{
		uint32_t size = bitmap.width() * bitmap.height();
		if ((m_usedspace + size) > (m_width * m_height)) // maybe try implemmenting code to increase size of atlas
			return false;
		if (!(m_ids.find(id) != m_ids.end()))
		{
			m_ids.insert({ id, {{0.0f, 0.0f}, {0.0f, 0.0f}} });
			m_data[size].push_back({ id, bitmap });
			m_usedspace += size;
		}
		else
			return false;
		return true;
	}

	template<typename id_type>
	inline tou::bitmap_image texture_atlas<id_type>::get()
	{
		uint32_t total_x_traveled = 0, total_y_traveled = 0;
		tou::bitmap_image atlas(m_width, m_height, m_background_color);
		uint32_t prev_height = 0, curr_height = 0;
		for (const auto& [image_size, vec] : m_data)
		{
			for (uint32_t i = 0; i < vec.size(); i++)
			{
				curr_height = vec[i].second.height();
				if (curr_height > prev_height)
					prev_height = curr_height;
				
				tou::ivec2 bottom_left = { total_x_traveled, total_y_traveled };
				if (bottom_left.x >= atlas.width() /* - 32*/ || bottom_left.x + vec[i].second.width() >= atlas.width() /* - 32*/)
				{
					bottom_left.x = 0;
					total_x_traveled = 0;
					total_y_traveled += prev_height;
					bottom_left.y = total_y_traveled;
				}
				tou::ivec2 atlas_pos = bottom_left;
				tou::ivec2 atlas_pos_top_right = { atlas_pos.x + vec[i].second.width(), atlas_pos.y + vec[i].second.height() };

				// record normalized element position in record entry for id
				m_ids[vec[i].first] =
				{
					{FLT(atlas_pos.x) / FLT(vec[i].second.width()), FLT(atlas_pos.y) / FLT(vec[i].second.height())},
					{FLT(atlas_pos_top_right.x) / FLT(vec[i].second.width()), FLT(atlas_pos_top_right.y) / FLT(vec[i].second.height())}
				};
				tou::ivec2 subimage_pos = { 0, 0 };
				for (uint32_t p = 0; p < image_size; p++)
				{
					if (subimage_pos.x == vec[i].second.width())
					{
						subimage_pos.x = 0;
						atlas_pos.x = bottom_left.x;
						subimage_pos.y++;
						atlas_pos.y++;
					}
					if (atlas_pos.x >= m_width || atlas_pos.y >= m_height)
						break;
					//atlas[atlas_pos] = vec[i].second[subimage_pos]; // doesnt work???
					atlas[atlas_pos] = m_data[image_size][i].second[subimage_pos];
					subimage_pos.x++;
					atlas_pos.x++;
				}
				total_x_traveled += vec[i].second.width();
			}
		}

		return atlas;
	}

	template<typename id_type>
	inline texture_atlas_element texture_atlas<id_type>::get_element(const id_type& id)
	{
		if (!(m_ids.find(id) != m_ids.end()))
		{
			return { {0.0f, 0.0f}, {0.0f, 0.0f} };
		}
		return m_ids[id];
	}
}


