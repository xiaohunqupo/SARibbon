﻿#include "SARibbonCategory.h"
#include <QList>
#include <QResizeEvent>
#include <QPainter>
#include <QLinearGradient>
#include <QApplication>
#include <QHBoxLayout>
#include <QList>
#include <QMap>
#include <QResizeEvent>
#include "SARibbonCategoryLayout.h"
#include "SARibbonElementManager.h"

#ifndef SARIBBONCATEGORY_DEBUG_PRINT
#define SARIBBONCATEGORY_DEBUG_PRINT 0
#endif
#if SARIBBONCATEGORY_DEBUG_PRINT
#include <QDebug>
#endif
///
/// \brief ribbon页的代理类
/// 如果需要修改重绘SARibbonCategory，可以通过设置SARibbonCategory::setProxy
///
class SARibbonCategory::PrivateData
{
	SA_RIBBON_DECLARE_PUBLIC(SARibbonCategory)
public:
	PrivateData(SARibbonCategory* p);

	SARibbonPannel* addPannel(const QString& title);
	SARibbonPannel* insertPannel(const QString& title, int index);
	void addPannel(SARibbonPannel* pannel);
	void insertPannel(int index, SARibbonPannel* pannel);

	// 把pannel从Category中移除，不会销毁，此时pannel的所有权归还操作者
	bool takePannel(SARibbonPannel* pannel);

	// 移除Pannel，Category会直接回收SARibbonPannel内存
	bool removePannel(SARibbonPannel* pannel);
	SARibbonCategory* ribbonCategory();
	const SARibbonCategory* ribbonCategory() const;

	// 返回所有的Pannel
	QList< SARibbonPannel* > pannelList();

	// 更新item的布局,此函数会调用doItemLayout
	void updateItemGeometry();

	void doWheelEvent(QWheelEvent* event);
	// 初始化
	void init(SARibbonCategory* c);

public:
	bool mEnableShowPannelTitle { true };    ///< 是否运行pannel的标题栏显示
	int mPannelTitleHeight { 15 };           ///< pannel的标题栏默认高度
	bool mIsContextCategory { false };       ///< 标记是否是上下文标签
	bool mIsCanCustomize { true };           ///< 标记是否可以自定义
	int mPannelSpacing { 0 };                ///< pannel的spacing
	bool m_isUseAnimating { true };          ///< 默认使用动画滚动
	QSize mPannelToolButtonSize { 22, 22 };  ///< 记录pannel的默认图标大小
	SARibbonPannel::PannelLayoutMode mDefaultPannelLayoutMode { SARibbonPannel::ThreeRowMode };
	int m_wheelScrollStep { 400 };  // 默认滚轮滚动步长
};
SARibbonCategory::PrivateData::PrivateData(SARibbonCategory* p) : q_ptr(p)
{
}

SARibbonPannel* SARibbonCategory::PrivateData::addPannel(const QString& title)
{
	if (SARibbonCategoryLayout* lay = q_ptr->categoryLayout()) {
		SARibbonPannel* p = insertPannel(title, lay->pannelCount());
		return p;
	}
	return nullptr;
}

SARibbonPannel* SARibbonCategory::PrivateData::insertPannel(const QString& title, int index)
{
	SARibbonPannel* pannel = RibbonSubElementFactory->createRibbonPannel(ribbonCategory());

	pannel->setPannelName(title);
	pannel->setObjectName(title);
	insertPannel(index, pannel);
	return (pannel);
}

void SARibbonCategory::PrivateData::addPannel(SARibbonPannel* pannel)
{
	if (SARibbonCategoryLayout* lay = q_ptr->categoryLayout()) {
		insertPannel(lay->pannelCount(), pannel);
	}
}

/**
 * @brief 插入pannel到layout
 *
 * 所有的添加操作最终会调用此函数
 * @param index
 * @param pannel
 */
void SARibbonCategory::PrivateData::insertPannel(int index, SARibbonPannel* pannel)
{
	if (nullptr == pannel) {
		return;
	}
	SARibbonCategoryLayout* lay = qobject_cast< SARibbonCategoryLayout* >(q_ptr->layout());
	if (nullptr == lay) {
		return;
	}
	if (pannel->parentWidget() != q_ptr) {
		pannel->setParent(q_ptr);
	}
	// 同步状态
	pannel->setEnableShowTitle(mEnableShowPannelTitle);
	pannel->setTitleHeight(mPannelTitleHeight);
	pannel->setPannelLayoutMode(mDefaultPannelLayoutMode);
	pannel->setSpacing(mPannelSpacing);
	pannel->setToolButtonIconSize(mPannelToolButtonSize);
	index = qMax(0, index);
	index = qMin(lay->pannelCount(), index);
	lay->insertPannel(index, pannel);
	pannel->setVisible(true);

	QObject::connect(pannel, &SARibbonPannel::actionTriggered, ribbonCategory(), &SARibbonCategory::actionTriggered);
	q_ptr->updateGeometry();  // 通知父布局这个控件的尺寸提示(sizeHint())可能已改变
}

bool SARibbonCategory::PrivateData::takePannel(SARibbonPannel* pannel)
{
	SARibbonCategoryLayout* lay = qobject_cast< SARibbonCategoryLayout* >(q_ptr->layout());
	if (nullptr == lay) {
		return false;
	}
	bool res = lay->takePannel(pannel);
	q_ptr->updateGeometry();  // 通知父布局这个控件的尺寸提示(sizeHint())可能已改变
	return res;
}

bool SARibbonCategory::PrivateData::removePannel(SARibbonPannel* pannel)
{
	if (takePannel(pannel)) {
		pannel->hide();
		pannel->deleteLater();
		return (true);
	}
	return (false);
}

QList< SARibbonPannel* > SARibbonCategory::PrivateData::pannelList()
{
	if (SARibbonCategoryLayout* lay = q_ptr->categoryLayout()) {
		return lay->pannelList();
	}
	return QList< SARibbonPannel* >();
}

SARibbonCategory* SARibbonCategory::PrivateData::ribbonCategory()
{
	return (q_ptr);
}

const SARibbonCategory* SARibbonCategory::PrivateData::ribbonCategory() const
{
	return (q_ptr);
}

void SARibbonCategory::PrivateData::updateItemGeometry()
{
#if SARIBBONCATEGORY_DEBUG_PRINT
	qDebug() << "SARibbonCategory::PrivateData::updateItemGeometry,categoryName=" << q_ptr->categoryName();
#endif
	SARibbonCategoryLayout* lay = qobject_cast< SARibbonCategoryLayout* >(q_ptr->layout());
	if (!lay) {
		return;
	}
	const QList< SARibbonPannel* > pannels = lay->pannelList();
	for (auto pannel : pannels) {
		pannel->updateItemGeometry();
	}
	lay->updateGeometryArr();
	return;
}

void SARibbonCategory::PrivateData::doWheelEvent(QWheelEvent* event)
{
	SARibbonCategoryLayout* lay = q_ptr->categoryLayout();
	if (nullptr == lay) {
		return;
	}

	// 如果动画正在进行，忽略新的事件
	if (m_isUseAnimating && lay->isAnimatingScroll()) {
		event->ignore();
		return;
	}

	QSize contentSize = lay->categoryContentSize();
	int totalWidth    = lay->categoryTotalWidth();

	if (totalWidth > contentSize.width()) {
		int scrollStep = m_wheelScrollStep;

		// 根据滚轮方向确定滚动方向
		QPoint numPixels  = event->pixelDelta();
		QPoint numDegrees = event->angleDelta() / 8;

		if (!numPixels.isNull()) {
			scrollStep = (numPixels.y() < 0) ? -scrollStep : scrollStep;
		} else if (!numDegrees.isNull()) {
			scrollStep = (numDegrees.y() < 0) ? -scrollStep : scrollStep;
		}

		// 动态调整步长 - 滚动越快步长越大
		const int absDelta = qMax(qAbs(numPixels.y()), qAbs(numDegrees.y()));
		if (absDelta > 60) {
			scrollStep *= 2;
		} else if (absDelta < 20) {
			scrollStep /= 2;
		}

		// 根据设置选择滚动方式
		if (m_isUseAnimating) {
			lay->scrollByAnimate(scrollStep);
		} else {
			lay->scroll(scrollStep);
		}
	} else {
		event->ignore();
		if (lay->isScrolled()) {
			// 根据设置选择复位方式
			if (m_isUseAnimating) {
				lay->scrollToByAnimate(0);
			} else {
				lay->scroll(0);
			}
		}
	}
}

void SARibbonCategory::PrivateData::init(SARibbonCategory* c)
{
	c->setLayout(new SARibbonCategoryLayout(c));
	c->connect(c, &SARibbonCategory::windowTitleChanged, c, &SARibbonCategory::categoryNameChanged);
}

//----------------------------------------------------
// SARibbonCategory
//----------------------------------------------------

SARibbonCategory::SARibbonCategory(QWidget* p) : QFrame(p), d_ptr(new SARibbonCategory::PrivateData(this))
{
    d_ptr->init(this);
}

/**
 * @brief 带名称的构造函数
 * @param name Category名称
 * @param p 父级控件指针
 */
SARibbonCategory::SARibbonCategory(const QString& name, QWidget* p)
    : QFrame(p), d_ptr(new SARibbonCategory::PrivateData(this))
{
	setCategoryName(name);
	d_ptr->init(this);
}

SARibbonCategory::~SARibbonCategory()
{
}

/**
 * @brief 获取Category名称
 * @return 当前Category的名称（即windowTitle）
 */
QString SARibbonCategory::categoryName() const
{
    return (windowTitle());
}

/**
 * @brief 设置Category名称
 * @param title 新名称（等价于setWindowTitle）
 */
void SARibbonCategory::setCategoryName(const QString& title)
{
    setWindowTitle(title);
}

bool SARibbonCategory::event(QEvent* e)
{
#if SARIBBONCATEGORY_DEBUG_PRINT
	if (e->type() != QEvent::Paint) {
		qDebug() << "SARibbonCategory event(" << e->type() << "),name=" << categoryName();
	}
#endif
	return QWidget::event(e);
}

/**
 * @brief 获取面板布局模式
 * @return 当前所有面板的布局模式
 */
SARibbonPannel::PannelLayoutMode SARibbonCategory::pannelLayoutMode() const
{
    return (d_ptr->mDefaultPannelLayoutMode);
}

/**
 * @brief 设置面板布局模式
 *
 * 在@ref SARibbonBar 调用@ref SARibbonBar::setRibbonStyle 函数时，会对所有的SARibbonCategory调用此函数
 * 把新的SARibbonPannel::PannelLayoutMode设置进去
 * @param m
 */
void SARibbonCategory::setPannelLayoutMode(SARibbonPannel::PannelLayoutMode m)
{
	d_ptr->mDefaultPannelLayoutMode = m;
	iteratePannel([ m ](SARibbonPannel* p) -> bool {
		p->setPannelLayoutMode(m);
		return true;
	});
	updateItemGeometry();
}

/**
 * @brief 添加面板(pannel)
 *
 * @note 面板(pannel)的所有权由SARibbonCategory来管理，请不要在外部对其进行销毁
 * @param title 面板(pannel)的标题，在office/wps的三行模式下会显示在面板(pannel)的下方
 * @return 返回生成的@ref SARibbonPannel 指针
 * @see 对SARibbonPannel的其他操作，参考 @ref SARibbonCategory::takePannel
 */
SARibbonPannel* SARibbonCategory::addPannel(const QString& title)
{
    return (d_ptr->addPannel(title));
}

/**
 * @brief 添加pannel
 * @param pannel pannel的所有权SARibbonCategory来管理
 */
void SARibbonCategory::addPannel(SARibbonPannel* pannel)
{
    d_ptr->addPannel(pannel);
}

/**
 * @brief qt designer专用
 * @param pannel
 */
void SARibbonCategory::addPannel(QWidget* pannel)
{
	SARibbonPannel* p = qobject_cast< SARibbonPannel* >(pannel);
	if (p) {
		addPannel(p);
	}
}

/**
 * @brief 新建一个pannel，并插入到index位置
 * @param title pannel的title
 * @param index 插入的位置，如果index超出category里pannel的个数，将插入到最后
 * @return 返回生成的@ref SARibbonPannel 指针
 * @note 如果
 */
SARibbonPannel* SARibbonCategory::insertPannel(const QString& title, int index)
{
    return (d_ptr->insertPannel(title, index));
}

/**
 * @brief 通过名字查找pannel
 * @param title
 * @return 如果有重名，只会返回第一个符合条件的
 */
SARibbonPannel* SARibbonCategory::pannelByName(const QString& title) const
{
	if (SARibbonCategoryLayout* lay = categoryLayout()) {
		return lay->pannelByName(title);
	}
	return nullptr;
}

/**
 * @brief 通过ObjectName查找pannel
 * @param objname
 * @return
 */
SARibbonPannel* SARibbonCategory::pannelByObjectName(const QString& objname) const
{
	if (SARibbonCategoryLayout* lay = categoryLayout()) {
		return lay->pannelByObjectName(objname);
	}
	return nullptr;
}

/**
 * @brief 通过索引找到pannel，如果超过索引范围，会返回nullptr
 * @param index
 * @return 如果超过索引范围，会返回nullptr
 */
SARibbonPannel* SARibbonCategory::pannelByIndex(int index) const
{
	if (SARibbonCategoryLayout* lay = categoryLayout()) {
		return lay->pannelByIndex(index);
	}
	return nullptr;
}

/**
 * @brief 查找pannel对应的索引
 * @param p
 * @return 如果找不到，返回-1
 */
int SARibbonCategory::pannelIndex(SARibbonPannel* p) const
{
	if (SARibbonCategoryLayout* lay = categoryLayout()) {
		return lay->pannelIndex(p);
	}
	return -1;
}

/**
 * @brief 移动一个Pannel从from index到to index
 * @param from 要移动pannel的index
 * @param to 要移动到的位置
 */
void SARibbonCategory::movePannel(int from, int to)
{
	if (from == to) {
		return;
	}
	if (to < 0) {
		to = 0;
	}
	if (to >= pannelCount()) {
		to = pannelCount() - 1;
	}
	if (SARibbonCategoryLayout* lay = categoryLayout()) {
		lay->movePannel(from, to);
	}
}

/**
 * @brief 把pannel脱离SARibbonCategory的管理
 * @param pannel 需要提取的pannel
 * @return 成功返回true，否则返回false
 */
bool SARibbonCategory::takePannel(SARibbonPannel* pannel)
{
    return (d_ptr->takePannel(pannel));
}

/**
 * @brief 移除Pannel，Category会直接回收SARibbonPannel内存
 * @param pannel 需要移除的pannel
 * @note 移除后pannel为野指针，一般操作完建议把pannel指针设置为nullptr
 *
 * 此操作等同于：
 *
 * @code
 * SARibbonPannel* pannel;
 * ...
 * category->takePannel(pannel);
 * pannel->hide();
 * pannel->deleteLater();
 * @endcode
 */
bool SARibbonCategory::removePannel(SARibbonPannel* pannel)
{
    return (d_ptr->removePannel(pannel));
}

/**
 * @brief 移除pannel
 * @param index pannel的索引，如果超出会返回false
 * @return 成功返回true
 */
bool SARibbonCategory::removePannel(int index)
{
	SARibbonPannel* p = pannelByIndex(index);

	if (nullptr == p) {
		return (false);
	}
	return (removePannel(p));
}

/**
 * @brief 返回Category下的所有pannel
 * @return
 */
QList< SARibbonPannel* > SARibbonCategory::pannelList() const
{
    return (d_ptr->pannelList());
}

QSize SARibbonCategory::sizeHint() const
{
	if (SARibbonCategoryLayout* lay = categoryLayout()) {
		return lay->sizeHint();
	}
	return QSize(1000, 100);
}

/**
 * @brief 如果是ContextCategory，此函数返回true
 * @return
 */
bool SARibbonCategory::isContextCategory() const
{
    return (d_ptr->mIsContextCategory);
}

/**
 * @brief 返回pannel的个数
 * @return
 */
int SARibbonCategory::pannelCount() const
{
	if (SARibbonCategoryLayout* lay = categoryLayout()) {
		return lay->pannelCount();
	}
	return -1;
}

/**
 * @brief 判断是否可以自定义
 * @return
 */
bool SARibbonCategory::isCanCustomize() const
{
    return (d_ptr->mIsCanCustomize);
}

/**
 * @brief 设置是否可以自定义
 * @param b
 */
void SARibbonCategory::setCanCustomize(bool b)
{
    d_ptr->mIsCanCustomize = b;
}

/**
 * @brief pannel标题栏的高度
 * @return
 */
int SARibbonCategory::pannelTitleHeight() const
{
    return d_ptr->mPannelTitleHeight;
}
/**
 * @brief 设置pannel的高度
 * @param h
 */
void SARibbonCategory::setPannelTitleHeight(int h)
{
	d_ptr->mPannelTitleHeight = h;
	iteratePannel([ h ](SARibbonPannel* p) -> bool {
		p->setTitleHeight(h);
		return true;
	});
}

/**
 * @brief 是否pannel显示标题栏
 * @return
 */
bool SARibbonCategory::isEnableShowPannelTitle() const
{
    return d_ptr->mEnableShowPannelTitle;
}

/**
 * @brief 设置显示pannel标题
 * @param on
 */
void SARibbonCategory::setEnableShowPannelTitle(bool on)
{
	d_ptr->mEnableShowPannelTitle = on;
	iteratePannel([ on ](SARibbonPannel* p) -> bool {
		p->setEnableShowTitle(on);
		return true;
	});
}

/**
 * @brief 获取对应的ribbonbar
 * @return 如果没有加入ribbonbar的管理，此值为null
 */
SARibbonBar* SARibbonCategory::ribbonBar() const
{
	// 第一个par是stackwidget
	if (QWidget* par = parentWidget()) {
		// 理论此时是ribbonbar
		par = par->parentWidget();
		while (par) {
			if (SARibbonBar* ribbon = qobject_cast< SARibbonBar* >(par)) {
				return ribbon;
			}
			par = par->parentWidget();
		}
	}
	return nullptr;
}

/**
 * @brief 刷新category的布局，适用于改变ribbon的模式之后调用
 */
void SARibbonCategory::updateItemGeometry()
{
#if SARIBBONCATEGORY_DEBUG_PRINT
	qDebug() << "SARibbonCategory name=" << categoryName() << " updateItemGeometry";
#endif
	d_ptr->updateItemGeometry();
}

/**
 * @brief 设置滚动时是否使用动画
 * @param useAnimating
 */
void SARibbonCategory::setUseAnimatingScroll(bool useAnimating)
{
    d_ptr->m_isUseAnimating = useAnimating;
}

/**
 * @brief 滚动时是否使用动画
 * @return
 */
bool SARibbonCategory::isUseAnimatingScroll() const
{
    return d_ptr->m_isUseAnimating;
}

/**
 * @brief 设置滚轮滚动步长（px）
 * @param step
 */
void SARibbonCategory::setWheelScrollStep(int step)
{
    d_ptr->m_wheelScrollStep = qMax(10, step);  // 最小10像素
}

/**
 * @brief 滚轮的滚动步长
 * @return
 */
int SARibbonCategory::wheelScrollStep() const
{
    return d_ptr->m_wheelScrollStep;
}

/**
 * @brief 设置动画持续时间
 * @param duration
 */
void SARibbonCategory::setAnimationDuration(int duration)
{
	if (SARibbonCategoryLayout* lay = categoryLayout()) {
		lay->setAnimationDuration(duration);
	}
}

/**
 * @brief 动画持续时间
 * @return
 */
int SARibbonCategory::animationDuration() const
{
	if (SARibbonCategoryLayout* lay = categoryLayout()) {
		return lay->animationDuration();
	}
	return -1;
}

/**
 * @brief 此函数会遍历Category下的所有pannel,执行函数指针
 * @param fp 函数指针返回false则停止迭代
 * @return 返回false代表没有迭代完所有的category，中途接收到回调函数的false返回而中断迭代
 */
bool SARibbonCategory::iteratePannel(FpPannelIterate fp) const
{
	const QList< SARibbonPannel* > ps = pannelList();
	for (SARibbonPannel* p : ps) {
		if (!fp(p)) {
			return false;
		}
	}
	return true;
}

/**
   @brief 设置Category的对齐方式
   @param al
 */
void SARibbonCategory::setCategoryAlignment(SARibbonAlignment al)
{
	SARibbonCategoryLayout* lay = qobject_cast< SARibbonCategoryLayout* >(layout());
	if (lay) {
		lay->setCategoryAlignment(al);
	}
}

/**
   @brief Category的对齐方式
   @return
 */
SARibbonAlignment SARibbonCategory::categoryAlignment() const
{
	SARibbonCategoryLayout* lay = qobject_cast< SARibbonCategoryLayout* >(layout());
	if (lay) {
		return lay->categoryAlignment();
	}
	return SARibbonAlignment::AlignLeft;
}

/**
 * @brief 设置pannel的spacing
 * @param n
 */
void SARibbonCategory::setPannelSpacing(int n)
{
	d_ptr->mPannelSpacing = n;
	iteratePannel([ n ](SARibbonPannel* pannel) -> bool {
		if (pannel) {
			pannel->setSpacing(n);
		}
		return true;
	});
}

/**
 * @brief pannel的spacing
 * @return
 */
int SARibbonCategory::pannelSpacing() const
{
    return d_ptr->mPannelSpacing;
}

/**
 * @brief 设置pannel按钮的icon尺寸，large action不受此尺寸影响
 * @param s
 */
void SARibbonCategory::setPannelToolButtonIconSize(const QSize& s)
{
	d_ptr->mPannelToolButtonSize = s;
	iteratePannel([ s ](SARibbonPannel* pannel) -> bool {
		if (pannel) {
			pannel->setToolButtonIconSize(s);
		}
		return true;
	});
}

/**
 * @brief pannel按钮的icon尺寸，large action不受此尺寸影响
 * @return
 */
QSize SARibbonCategory::pannelToolButtonIconSize() const
{
    return d_ptr->mPannelToolButtonSize;
}

/**
 * @brief 滚动事件
 *
 * 在内容超出category的宽度情况下，滚轮可滚动pannel
 * @param event
 */
void SARibbonCategory::wheelEvent(QWheelEvent* event)
{
    d_ptr->doWheelEvent(event);
}

void SARibbonCategory::changeEvent(QEvent* event)
{
	switch (event->type()) {
	case QEvent::StyleChange: {
		if (layout()) {
#if SARIBBONCATEGORY_DEBUG_PRINT
			qDebug() << "SARibbonCategory changeEvent(StyleChange),categoryName=" << categoryName();
#endif
			layout()->invalidate();
		}
	} break;
	case QEvent::FontChange: {
#if SARIBBONCATEGORY_DEBUG_PRINT
		qDebug() << "SARibbonCategory changeEvent(FontChange),categoryName=" << categoryName();
#endif
		QFont f = font();
		iteratePannel([ f ](SARibbonPannel* p) -> bool {
			p->setFont(f);
			return true;
		});
		if (layout()) {
			layout()->invalidate();
		}
	} break;
	default:
		break;
	};
	return QWidget::changeEvent(event);
}

/**
 * @brief 标记这个是上下文标签
 * @param isContextCategory
 */
void SARibbonCategory::markIsContextCategory(bool isContextCategory)
{
    d_ptr->mIsContextCategory = isContextCategory;
}

/**
 * @brief 获取SARibbonCategoryLayoutlayout
 * @return
 */
SARibbonCategoryLayout* SARibbonCategory::categoryLayout() const
{
    return qobject_cast< SARibbonCategoryLayout* >(layout());
}

//===================================================
// SARibbonCategoryScrollButton
//===================================================
SARibbonCategoryScrollButton::SARibbonCategoryScrollButton(Qt::ArrowType arr, QWidget* p) : QToolButton(p)
{
    setArrowType(arr);
}

SARibbonCategoryScrollButton::~SARibbonCategoryScrollButton()
{
}
