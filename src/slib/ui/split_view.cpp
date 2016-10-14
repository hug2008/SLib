#include "../../../inc/slib/ui/split_view.h"
#include "../../../inc/slib/ui/core.h"

SLIB_UI_NAMESPACE_BEGIN

SLIB_DEFINE_OBJECT(SplitView, View)

SplitView::SplitView()
{
	SLIB_REFERABLE_CONSTRUCTOR
	
	setCreatingChildInstances(sl_true);
	
	setCapturingChildInstanceEvents(sl_true);

	m_orientation = LayoutOrientation::Horizontal;
	
	m_dividerWidth = 1;
	m_dividerColor = Color::Gray;
	
	m_items.setCount(2);
	m_items.getItemPtr(0)->weight = 1;
	
#if defined(SLIB_PLATFORM_IS_DESKTOP)
	m_cursorMargin = 4;
#else
	m_cursorMargin = SLIB_MIN(UI::getScreenWidth(), UI::getScreenHeight()) / 60;
#endif
	
	m_indexDividerDown = -1;
	m_posDown = 0;
	m_weightDown = 0;
	
	m_cursor = Cursor::getResizeLeftRight();
	
}

SplitView::~SplitView()
{
}

SplitView::Item::Item()
{
	weight = 0;
	minWeight = 0;
	maxWeight = 1;
	minSize = 0;
	maxSize = 1000000;
	dividerWidth = 0;
	dividerColor = Color::zero();
	
	pos = 0;
	width = 0;
}

LayoutOrientation SplitView::getOrientation()
{
	return m_orientation;
}

void SplitView::setOrientation(LayoutOrientation orientation, sl_bool flagRelayout)
{
	if (m_orientation == orientation) {
		return;
	}
	m_orientation = orientation;
	ObjectLocker lock(this);
	if (flagRelayout) {
		_refreshItemFrames(sl_true);
	}
}

sl_bool SplitView::isHorizontal()
{
	return m_orientation == LayoutOrientation::Horizontal;
}

void SplitView::setHorizontal(sl_bool flagRelayout)
{
	setOrientation(LayoutOrientation::Horizontal, flagRelayout);
}

sl_bool SplitView::isVertical()
{
	return m_orientation == LayoutOrientation::Vertical;
}

void SplitView::setVertical(sl_bool flagRelayout)
{
	setOrientation(LayoutOrientation::Vertical, flagRelayout);
}

sl_size SplitView::getItemsCount()
{
	return m_items.getCount();
}

void SplitView::setItemsCount(sl_size count, sl_bool flagRelayout)
{
	if (count < 2) {
		return;
	}
	ObjectLocker lock(this);
	m_items.setCount_NoLock(count);
	if (flagRelayout) {
		_refreshItemFrames(sl_true);
	}
}

Ref<View> SplitView::getItemView(sl_size index)
{
	ObjectLocker lock(this);
	Item* item = m_items.getItemPtr(index);
	if (item) {
		return item->view;
	}
	return Ref<View>::null();
}

void SplitView::setItemView(sl_size index, const Ref<View>& view, sl_bool flagRelayout)
{
	ObjectLocker lock(this);
	Item* item = m_items.getItemPtr(index);
	if (item) {
		if (item->view.isNotNull()) {
			removeChild(item->view);
		}
		addChild(view);
		item->view = view;
		if (flagRelayout) {
			_refreshItemFrames(sl_true);
		}
	}
}

sl_ui_len SplitView::getItemSize(sl_size index)
{
	ObjectLocker lock(this);
	Item* item = m_items.getItemPtr(index);
	if (item) {
		return (sl_ui_len)(item->weight * _getTotalSize());
	}
	return 0;
}

void SplitView::setItemSize(sl_size index, sl_ui_len size, sl_bool flagRelayout)
{
	ObjectLocker lock(this);
	sl_ui_len total = _getTotalSize();
	if (total <= 0 || Math::isAlmostZero(total)) {
		return;
	}
	setItemWeight(index, (sl_real)size / (sl_real)total, flagRelayout);
}

sl_real SplitView::getItemWeight(sl_size index)
{
	ObjectLocker lock(this);
	Item* item = m_items.getItemPtr(index);
	if (item) {
		return item->weight;
	}
	return 0;
}

void SplitView::setItemWeight(sl_size index, sl_real weight, sl_bool flagRelayout)
{
	ObjectLocker lock(this);
	Item* item = m_items.getItemPtr(index);
	if (item) {
		sl_size nItems = m_items.getCount();
		Item* itemNext;
		if (index == nItems - 1) {
			index --;
			itemNext = item;
			item = m_items.getItemPtr(index);
			if (!item) {
				return;
			}
			weight = item->weight + itemNext->weight - weight;
		} else {
			itemNext = m_items.getItemPtr(index + 1);
			if (!itemNext) {
				return;
			}
		}
		sl_real weight1 = item->weight;
		sl_real weight2 = itemNext->weight;
		sl_real weightTotal = weight1 + weight2;
		sl_ui_len sizeTotal = _getTotalSize();
		sl_real minWeight1;
		sl_real maxWeight1;
		sl_real minWeight2;
		sl_real maxWeight2;
		if (sizeTotal <= 0 || Math::isAlmostZero(sizeTotal)) {
			minWeight1 = item->minWeight;
			maxWeight1 = item->maxWeight;
			minWeight2 = itemNext->minWeight;
			maxWeight2 = itemNext->maxWeight;
		} else {
			minWeight1 = (sl_real)(item->minSize) / (sl_real)sizeTotal;
			minWeight1 = SLIB_MAX(minWeight1, item->minWeight);
			maxWeight1 = (sl_real)(item->maxSize) / (sl_real)sizeTotal;
			maxWeight1 = SLIB_MIN(maxWeight1, item->maxWeight);
			minWeight2 = (sl_real)(itemNext->minSize) / (sl_real)sizeTotal;
			minWeight2 = SLIB_MAX(minWeight2, itemNext->minWeight);
			maxWeight2 = (sl_real)(itemNext->maxSize) / (sl_real)sizeTotal;
			maxWeight2 = SLIB_MIN(maxWeight2, itemNext->maxWeight);
		}
		if (weight < weightTotal - maxWeight2) {
			weight = weightTotal - maxWeight2;
		}
		if (weight > maxWeight1) {
			weight = maxWeight1;
		}
		if (weight > weightTotal - minWeight2) {
			weight = weightTotal - minWeight2;
		}
		if (weight < minWeight1) {
			weight = minWeight1;
		}
		if (weight < 0) {
			weight = 0;
		}
		if (weight > weightTotal) {
			weight = weightTotal;
		}
		item->weight = weight;
		itemNext->weight = weightTotal - weight;
		if (flagRelayout) {
			_refreshItemFrames(sl_true);
		}
	}
}

sl_real SplitView::getItemMinimumWeight(sl_size index)
{
	ObjectLocker lock(this);
	Item* item = m_items.getItemPtr(index);
	if (item) {
		return item->minWeight;
	}
	return 0;
}

void SplitView::setItemMinimumWeight(sl_size index, sl_real weight, sl_bool flagRelayout)
{
	ObjectLocker lock(this);
	Item* item = m_items.getItemPtr(index);
	if (item) {
		if (weight < 0) {
			weight = 0;
		}
		if (weight > 1) {
			weight = 1;
		}
		item->minWeight = weight;
		if (index > 0) {
			setItemWeight(index - 1, getItemWeight(index - 1), sl_false);
		}
		setItemWeight(index, item->weight, flagRelayout);
	}
}

sl_real SplitView::getItemMaximumWeight(sl_size index)
{
	ObjectLocker lock(this);
	Item* item = m_items.getItemPtr(index);
	if (item) {
		return item->maxWeight;
	}
	return 0;
}

void SplitView::setItemMaximumWeight(sl_size index, sl_real weight, sl_bool flagRelayout)
{
	ObjectLocker lock(this);
	Item* item = m_items.getItemPtr(index);
	if (item) {
		if (weight < 0) {
			weight = 0;
		}
		if (weight > 1) {
			weight = 1;
		}
		item->maxWeight = weight;
		if (index > 0) {
			setItemWeight(index - 1, getItemWeight(index - 1), sl_false);
		}
		setItemWeight(index, item->weight, flagRelayout);
	}
}

sl_ui_len SplitView::getItemMinimumSize(sl_size index)
{
	ObjectLocker lock(this);
	Item* item = m_items.getItemPtr(index);
	if (item) {
		return item->minSize;
	}
	return 0;
}

void SplitView::setItemMinimumSize(sl_size index, sl_ui_len size, sl_bool flagRelayout)
{
	ObjectLocker lock(this);
	Item* item = m_items.getItemPtr(index);
	if (item) {
		item->minSize = size;
		if (index > 0) {
			setItemWeight(index - 1, getItemWeight(index - 1), sl_false);
		}
		setItemWeight(index, item->weight, flagRelayout);
	}
}

sl_ui_len SplitView::getItemMaximumSize(sl_size index)
{
	ObjectLocker lock(this);
	Item* item = m_items.getItemPtr(index);
	if (item) {
		return item->maxSize;
	}
	return 0;
}

void SplitView::setItemMaximumSize(sl_size index, sl_ui_len size, sl_bool flagRelayout)
{
	ObjectLocker lock(this);
	Item* item = m_items.getItemPtr(index);
	if (item) {
		item->maxSize = size;
		if (index > 0) {
			setItemWeight(index - 1, getItemWeight(index - 1), sl_false);
		}
		setItemWeight(index, item->weight, flagRelayout);
	}
}

sl_ui_len SplitView::getItemDividerWidth(sl_size index)
{
	ObjectLocker lock(this);
	Item* item = m_items.getItemPtr(index);
	if (item) {
		return item->dividerWidth;
	}
	return 0;
}

void SplitView::setItemDividerWidth(sl_size index, sl_ui_len width, sl_bool flagRelayout)
{
	ObjectLocker lock(this);
	Item* item = m_items.getItemPtr(index);
	if (item) {
		item->dividerWidth = width;
		_resetWeights(flagRelayout);
	}
}

Ref<Drawable> SplitView::getItemDividerBackground(sl_size index)
{
	ObjectLocker lock(this);
	Item* item = m_items.getItemPtr(index);
	if (item) {
		return item->dividerBackground;
	}
	return 0;
}

void SplitView::setItemDividerBackground(sl_size index, const Ref<Drawable>& background, sl_bool flagRedraw)
{
	ObjectLocker lock(this);
	Item* item = m_items.getItemPtr(index);
	if (item) {
		item->dividerBackground = background;
		if (flagRedraw) {
			invalidate();
		}
	}
}

Color SplitView::getItemDividerColor(sl_size index)
{
	ObjectLocker lock(this);
	Item* item = m_items.getItemPtr(index);
	if (item) {
		return item->dividerColor;
	}
	return 0;
}

void SplitView::setItemDividerColor(sl_size index, const Color& color, sl_bool flagRedraw)
{
	ObjectLocker lock(this);
	Item* item = m_items.getItemPtr(index);
	if (item) {
		item->dividerColor = color;
		if (flagRedraw) {
			invalidate();
		}
	}
}

sl_ui_len SplitView::getDividerWidth()
{
	return m_dividerWidth;
}

void SplitView::setDividerWidth(sl_ui_len width, sl_bool flagRelayout)
{
	ObjectLocker lock(this);
	m_dividerWidth = width;
	_resetWeights(flagRelayout);
}

Ref<Drawable> SplitView::getDividerBackground()
{
	return m_dividerBackground;
}

void SplitView::setDividerBackground(const Ref<Drawable>& background, sl_bool flagRedraw)
{
	m_dividerBackground = background;
	if (flagRedraw) {
		invalidate();
	}
}

Color SplitView::getDividerColor()
{
	return m_dividerColor;
}

void SplitView::setDividerColor(const Color& color, sl_bool flagRedraw)
{
	m_dividerColor = color;
	if (flagRedraw) {
		invalidate();
	}
}

sl_ui_len SplitView::getCursorMargin()
{
	return m_cursorMargin;
}

void SplitView::setCursorMargin(sl_ui_len margin)
{
	m_cursorMargin = margin;
}

void SplitView::relayout(sl_bool flagRedraw)
{
	ObjectLocker lock(this);
	_refreshItemFrames(flagRedraw);
}

void SplitView::onResize(sl_ui_len width, sl_ui_len height)
{
	ObjectLocker lock(this);
	_resetWeights(sl_true);
}

void SplitView::onDraw(Canvas* canvas)
{
	ObjectLocker lock(this);
	LayoutOrientation orientation = m_orientation;
	sl_ui_pos height;
	if (orientation == LayoutOrientation::Horizontal) {
		height = getHeight();
	} else {
		height = getWidth();
	}
	ListItems<Item> items(m_items);
	for (sl_size i = 0; i + 1 < items.count; i++) {
		Item& item = items[i];
		sl_ui_pos dw = item.dividerWidth;
		if (dw == 0) {
			dw = m_dividerWidth;
		}
		if (dw > 0) {
			Color color = item.dividerColor;
			if (color.isZero()) {
				color = m_dividerColor;
			}
			Ref<Drawable> background = item.dividerBackground;
			if (background.isNull()) {
				background = m_dividerBackground;
			}
			if (color.isNotZero() || background.isNotNull()) {
				UIRect rc;
				if (orientation == LayoutOrientation::Horizontal) {
					rc.left = item.pos + item.width;
					rc.top = 0;
					rc.right = rc.left + dw;
					rc.bottom = height;
				} else {
					rc.left = 0;
					rc.top = item.pos + item.width;
					rc.right = height;
					rc.bottom = rc.top + dw;
				}
				if (color.isNotZero()) {
					canvas->fillRectangle(rc, color);
				}
				if (background.isNotNull()) {
					canvas->draw(rc, background);
				}
			}
		}
	}
}

void SplitView::onMeasureLayout(sl_bool flagHorizontal, sl_bool flagVertical)
{
}

void SplitView::onMakeLayout()
{
}

void SplitView::dispatchMouseEvent(UIEvent* ev)
{
	{
		UIAction action = ev->getAction();
		UIPoint pt = ev->getPoint();
		ObjectLocker lock(this);
		if (action == UIAction::LeftButtonDown) {
			sl_int32 index = _getDividerIndexAtPoint(pt);
			if (index >= 0) {
				m_weightDown = getItemWeight(index);
				if (m_orientation == LayoutOrientation::Horizontal) {
					m_posDown = pt.x;
				} else {
					m_posDown = pt.y;
				}
			}
			if (index >= 0) {
				m_indexDividerDown = index;
				return;
			} else {
				m_indexDividerDown = -1;
			}
		}
		if (action == UIAction::LeftButtonDrag || action == UIAction::LeftButtonUp) {
			sl_int32 index = m_indexDividerDown;
			if (index >= 0) {
				sl_real dx;
				if (m_orientation == LayoutOrientation::Horizontal) {
					dx = (sl_real)(pt.x - m_posDown) / (sl_real)(getWidth());
				} else {
					dx = (sl_real)(pt.y - m_posDown) / (sl_real)(getHeight());
				}
				setItemWeight(index, m_weightDown + dx);
				return;
			}
		}
	}
	
	View::dispatchMouseEvent(ev);
	
}

void SplitView::dispatchSetCursor(UIEvent* ev)
{
	{
		UIPoint pt = ev->getPoint();
		ObjectLocker lock(this);
		sl_int32 index = _getDividerIndexAtPoint(pt);
		if (index >= 0) {
			Cursor::setCurrent(m_cursor);
			ev->preventDefault();
			return;
		}
	}
	
	View::dispatchSetCursor(ev);
	
}

sl_bool SplitView::hitTestForCapturingChildInstanceEvents(const UIPoint& pt)
{
	ObjectLocker lock(this);
	sl_int32 index = _getDividerIndexAtPoint(pt);
	return index >= 0;
}

sl_ui_len SplitView::_getTotalSize()
{
	sl_ui_pos total;
	if (m_orientation == LayoutOrientation::Horizontal) {
		total = getWidth();
	} else {
		total = getHeight();
	}
	ListItems<Item> items(m_items);
	for (sl_size i = 0; i + 1 < items.count; i++) {
		Item& item = items[i];
		sl_ui_pos dw = item.dividerWidth;
		if (dw == 0) {
			dw = m_dividerWidth;
		}
		if (dw < 0) {
			dw = 0;
		}
		total -= dw;
	}
	if (total < 0) {
		total = 0;
	}
	return total;
}

void SplitView::_refreshItemFrames(sl_bool flagRedraw)
{
	LayoutOrientation orientation = m_orientation;
	sl_ui_pos total = _getTotalSize();
	sl_ui_pos pos = 0;
	sl_ui_pos height;
	if (orientation == LayoutOrientation::Horizontal) {
		height = getHeight();
	} else {
		height = getWidth();
	}
	
	ListItems<Item> items(m_items);
	for (sl_size i = 0; i < items.count; i++) {
		Item& item = items[i];
		sl_ui_pos width = (sl_ui_pos)(item.weight * total);
		if (width < 0) {
			width = 0;
		}
		if (item.view.isNotNull()) {
			if (orientation == LayoutOrientation::Horizontal) {
				item.view->setFrame(UIRect(pos, 0, pos + width, height), sl_false);
			} else {
				item.view->setFrame(UIRect(0, pos, height, pos + width), sl_false);
			}
		}
		
		item.pos = pos;
		item.width = width;
		
		pos += width;
		sl_ui_pos dw = item.dividerWidth;
		if (dw == 0) {
			dw = m_dividerWidth;
		}
		if (dw < 0) {
			dw = 0;
		}
		pos += dw;
	}

	if (flagRedraw) {
		invalidate();
	}
}

void SplitView::_resetWeights(sl_bool flagRelayout)
{
	ListItems<Item> items(m_items);
	for (sl_size i = 0; i + 1 < items.count; i++) {
		setItemWeight(i, items[i].weight, sl_false);
	}
	if (flagRelayout) {
		_refreshItemFrames(sl_true);
	}
}

sl_int32 SplitView::_getDividerIndexAtPoint(const UIPoint& pt)
{
	LayoutOrientation orientation = m_orientation;
	sl_ui_pos height;
	if (orientation == LayoutOrientation::Horizontal) {
		height = getHeight();
	} else {
		height = getWidth();
	}
	sl_ui_pos margin = m_cursorMargin;
	ListItems<Item> items(m_items);
	for (sl_size i = 0; i + 1 < items.count; i++) {
		Item& item = items[i];
		sl_ui_pos dw = item.dividerWidth;
		if (dw == 0) {
			dw = m_dividerWidth;
		}
		if (dw < 0) {
			dw = 0;
		}
		UIRect rc;
		if (orientation == LayoutOrientation::Horizontal) {
			rc.left = item.pos + item.width;
			rc.top = 0;
			rc.right = rc.left + dw;
			rc.bottom = height;
			rc.left -= margin;
			rc.right += margin;
		} else {
			rc.left = 0;
			rc.top = item.pos + item.width;
			rc.right = height;
			rc.bottom = rc.top + dw;
			rc.top -= margin;
			rc.bottom += margin;
		}
		if (rc.containsPoint(pt)) {
			return (sl_int32)i;
		}
	}
	return -1;
}

VerticalSplitView::VerticalSplitView()
{
	setOrientation(LayoutOrientation::Vertical, sl_false);
}

HorizontalSplitView::HorizontalSplitView()
{
	setOrientation(LayoutOrientation::Horizontal, sl_false);
}

SLIB_UI_NAMESPACE_END