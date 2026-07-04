#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "ClassParser.h"
#include "LayoutEngine.h"

#include <QPlainTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QTextOption>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QSplitter>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsPathItem>
#include <QGraphicsPolygonItem>
#include <QTableWidget>
#include <QHeaderView>
#include <QListWidget>
#include <QTabWidget>
#include <QLabel>
#include <QFrame>
#include <QComboBox>
#include <QApplication>
#include <QDialog>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QFontMetrics>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QGraphicsDropShadowEffect>
#include <QScrollBar>
#include <QRegularExpression>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QPointer>
#include <memory>
#include <algorithm>
#include <functional>

namespace {

QString inheritancePathText(const string &path);
QString baseSummaryText(const ClassLayout &layoutInfo);
int directBaseCount(const ClassLayout &layoutInfo);

struct AiRequestConfig {
    QString provider;
    QString model;
    QUrl url;
    QByteArray apiKey;
    QJsonObject body;
    bool needsApiKey;
};

void applyModernStyle() {
    qApp->setStyleSheet(R"(
QMainWindow {
    background-color: #030712;
}

QWidget {
    font-family: "Helvetica Neue", "Arial";
    font-size: 12px;
    color: #dbeafe;
}

QLabel#TitleLabel {
    color: #f8fbff;
    font-size: 16px;
    font-weight: bold;
}

QLabel#SubTitleLabel {
    color: #8fb5d9;
    font-size: 10px;
}

QFrame#Panel {
    background-color: rgba(4, 12, 27, 238);
    border: 1px solid #0b3458;
    border-radius: 10px;
}

QPlainTextEdit, QTextEdit {
    background-color: #020817;
    color: #dbeafe;
    border: 1px solid #0b5a91;
    border-radius: 9px;
    padding: 8px;
    selection-background-color: #0369a1;
}

QPushButton {
    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #0ea5e9, stop:1 #2563eb);
    color: white;
    border: 1px solid #38bdf8;
    border-radius: 8px;
    padding: 8px 14px;
    font-weight: bold;
}

QPushButton:hover {
    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #22d3ee, stop:1 #2563eb);
}

QPushButton:pressed {
    background-color: #1e40af;
}

QPushButton#SecondaryButton {
    background-color: #172a42;
    border: 1px solid #31597d;
}

QPushButton#SecondaryButton:hover {
    background-color: #203b59;
}

QListWidget {
    background-color: #020817;
    border: 1px solid #0b5a91;
    border-radius: 9px;
    padding: 7px;
    outline: none;
}

QListWidget::item {
    padding: 8px;
    margin: 4px 0;
    border-radius: 9px;
    color: #dbeafe;
}

QListWidget::item:selected {
    background-color: #075985;
    border: 1px solid #38bdf8;
    color: white;
}

QTabWidget::pane {
    border: 1px solid #17446d;
    border-radius: 16px;
    background: #030817;
}

QTabBar::tab {
    background: #13243a;
    color: #a7c8e9;
    padding: 10px 22px;
    border-top-left-radius: 10px;
    border-top-right-radius: 10px;
    font-weight: bold;
}

QTabBar::tab:selected {
    background: #0877ff;
    color: white;
}

QTableWidget {
    background-color: #030817;
    alternate-background-color: #061326;
    color: #e5e7eb;
    gridline-color: #17446d;
    border: none;
    border-radius: 0px;
}

QTableWidget::viewport {
    background-color: #030817;
}

QTableWidget::item {
    border-color: #10233a;
}

QHeaderView {
    background-color: #10233a;
}

QHeaderView::section {
    background-color: #10233a;
    color: #e5e7eb;
    padding: 6px;
    border: 1px solid #0b2743;
}

QTableCornerButton::section {
    background-color: #10233a;
    border: 1px solid #0b2743;
}

QFrame#TableFrame {
    background-color: #071426;
    border: 1px solid #17446d;
    border-radius: 12px;
}

QGraphicsView {
    background-color: #020817;
    border: 1px solid #0b5a91;
    border-radius: 10px;
}

QScrollBar:vertical, QScrollBar:horizontal {
    background: #071426;
    border: none;
    width: 10px;
    height: 10px;
}

QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
    background: #2563eb;
    border-radius: 5px;
}
)");
}

QFrame *makePanel(QWidget *parent = nullptr) {
    QFrame *frame = new QFrame(parent);
    frame->setObjectName("Panel");
    return frame;
}

QLabel *makeTitle(const QString &text, QWidget *parent = nullptr) {
    QLabel *label = new QLabel(text, parent);
    label->setObjectName("TitleLabel");
    return label;
}

QLabel *makeSubTitle(const QString &text, QWidget *parent = nullptr) {
    QLabel *label = new QLabel(text, parent);
    label->setObjectName("SubTitleLabel");
    return label;
}

QColor fieldColor(const FieldInfo &field) {
    if (field.isVptr) return QColor("#f59e0b");
    if (field.isPadding) return QColor("#7c3aed");
    return QColor("#0ea5e9");
}

QColor fieldGlowColor(const FieldInfo &field) {
    if (field.isVptr) return QColor("#f59e0b");
    if (field.isPadding) return QColor("#a855f7");
    return QColor("#0284c7");
}

void addSceneText(QGraphicsScene *scene,
                  const QString &text,
                  qreal x,
                  qreal y,
                  int pointSize = 10,
                  const QColor &color = QColor("#e5e7eb")) {
    QGraphicsTextItem *item = scene->addText(text);
    QFont font = item->font();
    font.setFamily("Helvetica Neue");
    font.setPointSize(pointSize);
    item->setFont(font);
    item->setDefaultTextColor(color);
    item->setPos(x, y);
}

QGraphicsTextItem *addBoldText(QGraphicsScene *scene,
                               const QString &text,
                               qreal x,
                               qreal y,
                               int pointSize,
                               const QColor &color) {
    QGraphicsTextItem *item = scene->addText(text);
    QFont font = item->font();
    font.setFamily("Helvetica Neue");
    font.setPointSize(pointSize);
    font.setBold(true);
    item->setFont(font);
    item->setDefaultTextColor(color);
    item->setPos(x, y);
    return item;
}

QString elideToWidth(const QString &text, const QFont &font, int width) {
    QFontMetrics metrics(font);
    return metrics.elidedText(text, Qt::ElideRight, width);
}

QGraphicsTextItem *addWrappedText(QGraphicsScene *scene,
                                  const QString &text,
                                  const QRectF &rect,
                                  int pointSize,
                                  const QColor &color,
                                  bool bold = false) {
    QGraphicsTextItem *item = scene->addText(text);
    QFont font = item->font();
    font.setFamily("Helvetica Neue");
    font.setPointSize(pointSize);
    font.setBold(bold);
    item->setFont(font);
    item->setDefaultTextColor(color);
    item->setTextWidth(rect.width());
    QTextOption option = item->document()->defaultTextOption();
    option.setWrapMode(QTextOption::WrapAnywhere);
    item->document()->setDefaultTextOption(option);
    item->setPos(rect.topLeft());
    return item;
}

QString vtableFunctionLabel(const VTableEntry &entry) {
    QString className = QString::fromStdString(entry.actualClass);
    QString functionName = QString::fromStdString(entry.functionName) + "()";
    return className + "::" + functionName;
}

void addVTableFunctionText(QGraphicsScene *scene,
                           const VTableEntry &entry,
                           const QRectF &rect,
                           int pointSize,
                           const QColor &color) {
    QFont font;
    font.setFamily("Helvetica Neue");
    font.setPointSize(pointSize);
    QFontMetrics metrics(font);

    QString oneLine = vtableFunctionLabel(entry);
    if (metrics.horizontalAdvance(oneLine) <= rect.width()) {
        addSceneText(scene, oneLine, rect.x(), rect.y(), pointSize, color);
        return;
    }

    QString classLine = QString::fromStdString(entry.actualClass) + "::";
    QString functionLine = QString::fromStdString(entry.functionName) + "()";
    addSceneText(scene,
                 elideToWidth(classLine, font, static_cast<int>(rect.width())),
                 rect.x(),
                 rect.y() - 5,
                 pointSize,
                 color);
    addSceneText(scene,
                 elideToWidth(functionLine, font, static_cast<int>(rect.width())),
                 rect.x(),
                 rect.y() + pointSize + 4,
                 pointSize,
                 color);
}

QPainterPath roundedRectPath(const QRectF &rect, qreal radius) {
    QPainterPath path;
    path.addRoundedRect(rect, radius, radius);
    return path;
}

void addGlowRect(QGraphicsScene *scene,
                 const QRectF &rect,
                 const QColor &color,
                 qreal radius,
                 qreal width = 10) {
    QColor glow = color;
    glow.setAlpha(80);
    QGraphicsPathItem *outer = scene->addPath(roundedRectPath(rect.adjusted(-4, -4, 4, 4), radius + 4));
    outer->setPen(QPen(glow, width));
    outer->setBrush(Qt::NoBrush);

    glow.setAlpha(34);
    QGraphicsPathItem *halo = scene->addPath(roundedRectPath(rect.adjusted(-11, -11, 11, 11), radius + 8));
    halo->setPen(QPen(glow, width + 8));
    halo->setBrush(Qt::NoBrush);
}

void addGradientPanel(QGraphicsScene *scene,
                      const QRectF &rect,
                      const QColor &border,
                      const QColor &top,
                      const QColor &bottom,
                      qreal radius = 14) {
    QLinearGradient bg(rect.topLeft(), rect.bottomRight());
    bg.setColorAt(0.0, top);
    bg.setColorAt(1.0, bottom);
    addGlowRect(scene, rect, border, radius, 7);
    QGraphicsPathItem *panel = scene->addPath(roundedRectPath(rect, radius));
    panel->setPen(QPen(border, 1.5));
    panel->setBrush(QBrush(bg));
}

void addPanelGlow(QWidget *widget, const QColor &color) {
    auto *effect = new QGraphicsDropShadowEffect(widget);
    effect->setBlurRadius(28);
    effect->setOffset(0, 0);
    effect->setColor(color);
    widget->setGraphicsEffect(effect);
}

QPointF addMemoryBlock3D(QGraphicsScene *scene,
                         const FieldInfo &field,
                         qreal x,
                         qreal y,
                         qreal w,
                         qreal h) {
    const qreal depthX = 12;
    const qreal depthY = -7;
    QColor base = fieldColor(field);
    QColor glow = fieldGlowColor(field);

    QRectF front(x, y, w, h);
    QPolygonF topFace;
    topFace << QPointF(x, y)
            << QPointF(x + depthX, y + depthY)
            << QPointF(x + w + depthX, y + depthY)
            << QPointF(x + w, y);

    QPolygonF sideFace;
    sideFace << QPointF(x + w, y)
             << QPointF(x + w + depthX, y + depthY)
             << QPointF(x + w + depthX, y + h + depthY)
             << QPointF(x + w, y + h);

    QColor top = base.lighter(145);
    top.setAlpha(185);
    QColor side = base.darker(160);
    side.setAlpha(178);
    QColor frontColor = base;
    frontColor.setAlpha(172);

    addGlowRect(scene, front.adjusted(-2, -2, depthX + 2, 2), glow, 8, 8);

    QGraphicsPolygonItem *topItem = scene->addPolygon(topFace, QPen(glow.lighter(135), 1.2), QBrush(top));
    QGraphicsPolygonItem *sideItem = scene->addPolygon(sideFace, QPen(glow.lighter(135), 1.2), QBrush(side));

    QLinearGradient frontGradient(front.topLeft(), front.bottomRight());
    frontGradient.setColorAt(0.0, frontColor.lighter(125));
    frontGradient.setColorAt(1.0, frontColor.darker(135));
    QGraphicsPathItem *frontItem = scene->addPath(roundedRectPath(front, 7));
    frontItem->setPen(QPen(glow.lighter(165), 1.2));
    frontItem->setBrush(QBrush(frontGradient));

    Q_UNUSED(topItem);
    Q_UNUSED(sideItem);

    QString typeText;
    if (field.isVptr) {
        typeText = "vptr";
    } else if (field.isPadding) {
        typeText = "padding";
    } else {
        typeText = QString::fromStdString(field.typeName) + " " + QString::fromStdString(field.name);
    }

    const int textWidth = static_cast<int>(w - 44);
    if (h >= 58) {
        QFont metaFont("Helvetica Neue");
        metaFont.setPointSize(14);
        QFont nameFont("Helvetica Neue");
        nameFont.setPointSize(17);
        nameFont.setBold(true);

        QString meta = QString("0x%1    %2 bytes")
                           .arg(field.offset, 2, 16, QChar('0')).toUpper()
                           .arg(field.size);
        addSceneText(scene,
                     elideToWidth(meta, metaFont, textWidth),
                     x + 22,
                     y + 5,
                     14,
                     QColor("#e0f2fe"));
        addBoldText(scene,
                    elideToWidth(typeText, nameFont, textWidth),
                    x + 22,
                    y + h - 38,
                    17,
                    QColor("#ffffff"));
    } else {
        QFont compactFont("Helvetica Neue");
        compactFont.setPointSize(14);
        compactFont.setBold(true);
        QString compact = QString("0x%1  %2  %3B")
                              .arg(field.offset, 2, 16, QChar('0')).toUpper()
                              .arg(typeText)
                              .arg(field.size);
        addBoldText(scene,
                    elideToWidth(compact, compactFont, textWidth),
                    x + 18,
                    y + h * 0.5 - 16,
                    14,
                    QColor("#ffffff"));
    }

    return QPointF(x + w + depthX, y + h * 0.5 + depthY * 0.5);
}

bool classHasVirtual(const ClassLayout &layoutInfo) {
    for (const FieldInfo &field : layoutInfo.fields) {
        if (field.isVptr) return true;
    }
    for (const FunctionInfo &fn : layoutInfo.functions) {
        if (fn.isVirtual) return true;
    }
    return false;
}

int fieldPageCount(const ClassLayout &layoutInfo) {
    const int fieldsPerPage = 6;
    int count = static_cast<int>(layoutInfo.fields.size());
    return std::max(1, (count + fieldsPerPage - 1) / fieldsPerPage);
}

int vtablePageCount(const ClassLayout &layoutInfo) {
    const int entriesPerPage = 3;
    int count = static_cast<int>(layoutInfo.vtable.size());
    return std::max(1, (count + entriesPerPage - 1) / entriesPerPage);
}

void drawMemoryLayout(QGraphicsScene *scene,
                      const ClassLayout &layoutInfo,
                      const QSize &viewSize,
                      int fieldPage,
                      int vtablePage) {
    scene->clear();

    qreal viewW = std::max(1, viewSize.width());
    qreal viewH = std::max(1, viewSize.height());
    qreal viewAspect = viewW / viewH;
    const qreal canvasW = 1120;
    const qreal canvasH = std::max(620.0, std::min(820.0, canvasW / viewAspect));
    const QRectF canvas(0, 0, canvasW, canvasH);
    QLinearGradient background(canvas.topLeft(), canvas.bottomRight());
    background.setColorAt(0.0, QColor("#020617"));
    background.setColorAt(0.52, QColor("#031a2e"));
    background.setColorAt(1.0, QColor("#05030f"));
    scene->addRect(canvas, Qt::NoPen, QBrush(background));

    QPen gridPen(QColor(20, 184, 255, 22), 1);
    for (int x = 34; x < canvas.width(); x += 32) {
        scene->addLine(x, 0, x, canvas.height(), gridPen);
    }
    for (int y = 34; y < canvas.height(); y += 32) {
        scene->addLine(0, y, canvas.width(), y, gridPen);
    }

    QString className = QString::fromStdString(layoutInfo.className);
    addBoldText(scene, "Object Memory Layout", 42, 34, 28, QColor("#f8fbff"));
    addSceneText(scene, "Offset", 50, 108, 17, QColor("#8fb5d9"));

    QRectF sizeBadge(canvas.right() - 300, 34, 250, 50);
    addGradientPanel(scene, sizeBadge, QColor("#0284c7"), QColor("#04233d"), QColor("#06111f"), 10);
    addWrappedText(scene,
                   QString("sizeof(%1)\n= %2 bytes").arg(className).arg(layoutInfo.totalSize),
                   sizeBadge.adjusted(16, 3, -12, -2),
                   15,
                   QColor("#22d3ee"),
                   true);

    if (layoutInfo.fields.empty()) {
        addSceneText(scene, "No fields found.", 46, 110, 12, QColor("#fca5a5"));
        return;
    }

    const int fieldsPerPage = 6;
    const int pageCount = fieldPageCount(layoutInfo);
    fieldPage = std::max(0, std::min(fieldPage, pageCount - 1));
    const int firstField = fieldPage * fieldsPerPage;
    const int lastField = std::min(firstField + fieldsPerPage,
                                   static_cast<int>(layoutInfo.fields.size()));

    const qreal stackX = canvas.width() * 0.24;
    qreal stackY = 130;
    const qreal blockWidth = canvas.width() * 0.31;
    const qreal gap = 16;
    const qreal minHeight = 54;
    const qreal maxHeight = 82;
    const qreal availableHeight = canvas.height() - 250;
    QVector<qreal> heights;
    qreal naturalTotal = 0;
    for (int i = firstField; i < lastField; ++i) {
        const FieldInfo &field = layoutInfo.fields[i];
        qreal natural = std::max(58.0, std::min(maxHeight, 38.0 + field.size * 4.2));
        heights.append(natural);
        naturalTotal += natural;
    }

    qreal heightScale = 1.0;
    if (!heights.empty()) {
        qreal gaps = gap * std::max(0, static_cast<int>(heights.size()) - 1);
        heightScale = std::min(1.0, (availableHeight - gaps) / naturalTotal);
    }

    struct VisibleVptrAnchor {
        QPointF point;
        QString label;
    };
    QVector<VisibleVptrAnchor> visibleVptrAnchors;
    QStringList allVptrLabels;

    for (const FieldInfo &field : layoutInfo.fields) {
        if (!field.isVptr) continue;
        QString label = field.ownerPath.empty()
                            ? QString::fromStdString(field.ownerClass)
                            : inheritancePathText(field.ownerPath);
        if (label.isEmpty()) label = "vptr";
        if (!allVptrLabels.contains(label)) allVptrLabels.append(label);
    }

    for (int i = firstField; i < lastField; ++i) {
        const FieldInfo &field = layoutInfo.fields[i];
        qreal h = std::max(minHeight, heights[i - firstField] * heightScale);
        QPointF rightAnchor = addMemoryBlock3D(scene, field, stackX, stackY, blockWidth, h);

        addSceneText(scene,
                     QString("0x%1").arg(field.offset, 2, 16, QChar('0')).toUpper(),
                     62,
                     stackY + h * 0.5 - 13,
                     17,
                     QColor("#b6d8ff"));

        if (field.isVptr) {
            VisibleVptrAnchor anchor;
            anchor.point = rightAnchor;
            anchor.label = field.ownerPath.empty()
                               ? QString::fromStdString(field.ownerClass)
                               : inheritancePathText(field.ownerPath);
            if (anchor.label.isEmpty()) anchor.label = "vptr";
            visibleVptrAnchors.append(anchor);
        }

        stackY += h + gap;
    }

    if (pageCount > 1) {
        addBoldText(scene,
                    QString("Fields page %1 / %2").arg(fieldPage + 1).arg(pageCount),
                    stackX,
                    84,
                    16,
                    QColor("#7dd3fc"));
    }

    QRectF platform(stackX - 36, stackY + 4, blockWidth + 96, 38);
    QRadialGradient platformGlow(platform.center(), platform.width() * 0.55);
    platformGlow.setColorAt(0.0, QColor(14, 165, 233, 145));
    platformGlow.setColorAt(0.55, QColor(14, 165, 233, 45));
    platformGlow.setColorAt(1.0, QColor(14, 165, 233, 0));
    scene->addEllipse(platform, Qt::NoPen, QBrush(platformGlow));
    scene->addEllipse(platform.adjusted(28, 10, -28, -10), QPen(QColor("#38bdf8"), 1.2), Qt::NoBrush);

    const qreal rightX = canvas.width() * 0.705;
    const qreal rightW = std::max(280.0, canvas.right() - rightX - 50);
    const bool splitVTables = allVptrLabels.size() > 1;

    const int entriesPerVTablePage = 3;
    const int vtableRows = static_cast<int>(layoutInfo.vtable.size());
    const int vtablePages = vtablePageCount(layoutInfo);
    vtablePage = std::max(0, std::min(vtablePage, vtablePages - 1));
    const int firstVTableRow = vtablePage * entriesPerVTablePage;
    const int lastVTableRow = std::min(firstVTableRow + entriesPerVTablePage, vtableRows);
    const int visibleVTableRows = std::min(entriesPerVTablePage, std::max(1, lastVTableRow - firstVTableRow));
    const qreal rowHeight = 72;
    const qreal rowGap = 8;
    const qreal vtableHeight = classHasVirtual(layoutInfo)
                                   ? 132.0 + visibleVTableRows * (rowHeight + rowGap)
                                   : 118.0;
    QRectF vtablePanel(rightX, canvas.height() * 0.27, rightW, vtableHeight);
    QVector<QRectF> splitPanels;

    if (splitVTables) {
        const qreal panelGap = 14;
        const qreal splitRowHeight = 44;
        qreal panelY = vtablePanel.y();

        for (int i = 0; i < allVptrLabels.size(); ++i) {
            QString vptrLabel = allVptrLabels[i];
            QString baseName = vptrLabel.section("::", -1);
            QVector<const VTableEntry *> entries;
            for (const VTableEntry &entry : layoutInfo.vtable) {
                if (QString::fromStdString(entry.ownerClass) == baseName ||
                    QString::fromStdString(entry.actualClass) == baseName) {
                    entries.append(&entry);
                }
            }

            const int rowCount = std::max(1, static_cast<int>(entries.size()));
            const qreal splitPanelHeight = 76 + rowCount * (splitRowHeight + 7) + 8;
            QRectF panel(rightX,
                         panelY,
                         rightW,
                         splitPanelHeight);
            panelY += splitPanelHeight + panelGap;
            splitPanels.append(panel);
            addGradientPanel(scene, panel, QColor("#a855f7"), QColor("#140924"), QColor("#0b1022"), 12);
            addBoldText(scene,
                        vptrLabel + " Virtual Table",
                        panel.x() + 16,
                        panel.y() + 10,
                        15,
                        QColor("#f5f3ff"));
            addSceneText(scene, "Index", panel.x() + 18, panel.y() + 42, 12, QColor("#c4b5fd"));
            addSceneText(scene, "Function", panel.x() + 60, panel.y() + 42, 12, QColor("#c4b5fd"));
            addSceneText(scene, "Dispatch", panel.right() - 74, panel.y() + 42, 12, QColor("#c4b5fd"));

            if (entries.isEmpty()) {
                addSceneText(scene,
                             "No entries parsed for this subobject.",
                             panel.x() + 16,
                             panel.y() + 70,
                             11,
                             QColor("#facc15"));
            }

            for (int entryIndex = 0; entryIndex < entries.size(); ++entryIndex) {
                const VTableEntry &entry = *entries[entryIndex];
                QRectF r(panel.x() + 14,
                         panel.y() + 64 + entryIndex * (splitRowHeight + 7),
                         panel.width() - 28,
                         splitRowHeight);
                QGraphicsPathItem *rowBg = scene->addPath(roundedRectPath(r, 7));
                rowBg->setPen(QPen(QColor(168, 85, 247, 70)));
                rowBg->setBrush(QBrush(QColor(30, 41, 59, 150)));

                addBoldText(scene, QString::number(entry.index), r.x() + 12, r.y() + 9, 14, QColor("#f5d0fe"));

                addVTableFunctionText(scene,
                                      entry,
                                      QRectF(r.x() + 50, r.y() + 8, r.width() - 122, r.height() - 8),
                                      12,
                                      QColor("#ffffff"));

                addSceneText(scene,
                             entry.isOverride ? "override" : "base",
                             r.right() - 68,
                             r.y() + 7,
                             12,
                             entry.isOverride ? QColor("#fbbf24") : QColor("#93c5fd"));
            }
        }
    } else if (classHasVirtual(layoutInfo)) {
        addGradientPanel(scene, vtablePanel, QColor("#a855f7"), QColor("#140924"), QColor("#0b1022"), 12);
        addBoldText(scene, "Virtual Table",
                    vtablePanel.x() + 18,
                    vtablePanel.y() + 12,
                    18,
                    QColor("#f5f3ff"));
        addSceneText(scene,
                     QString("%1 vtable    page %2 / %3")
                         .arg(className)
                         .arg(vtablePage + 1)
                         .arg(vtablePages),
                     vtablePanel.x() + 18,
                     vtablePanel.y() + 42,
                     13,
                     QColor("#c4b5fd"));

        QRectF header(vtablePanel.x() + 16, vtablePanel.y() + 76, vtablePanel.width() - 32, 34);
        scene->addRect(header, QPen(QColor(168, 85, 247, 90)), QBrush(QColor(15, 23, 42, 190)));
        const qreal indexX = header.x() + 12;
        const qreal functionX = header.x() + 58;
        const qreal dispatchX = header.right() - 78;
        addSceneText(scene, "Index", indexX, header.y() + 1, 14, QColor("#c4b5fd"));
        addSceneText(scene, "Function", functionX, header.y() + 1, 14, QColor("#c4b5fd"));
        addSceneText(scene, "Dispatch", dispatchX, header.y() + 1, 14, QColor("#c4b5fd"));

        int row = 0;
        if (!layoutInfo.vtable.empty()) {
            for (int i = firstVTableRow; i < lastVTableRow; ++i) {
                const VTableEntry &entry = layoutInfo.vtable[i];
                QRectF r(header.x(), header.y() + 48 + row * (rowHeight + rowGap), header.width(), rowHeight);
                QColor rowColor = entry.isOverride ? QColor(251, 191, 36, 38) : QColor(30, 41, 59, 150);
                QGraphicsPathItem *rowBg = scene->addPath(roundedRectPath(r, 7));
                rowBg->setPen(QPen(QColor(168, 85, 247, 70)));
                rowBg->setBrush(QBrush(rowColor));

                addBoldText(scene, QString::number(entry.index), r.x() + 14, r.y() + 17, 17, QColor("#f5d0fe"));
                QRectF functionRect(functionX, r.y() + 7, std::max(102.0, dispatchX - functionX - 12), rowHeight - 14);
                QRectF dispatchRect(dispatchX, r.y() + 23, std::max(56.0, r.right() - dispatchX - 10), rowHeight - 18);
                addVTableFunctionText(scene,
                                      entry,
                                      functionRect.adjusted(0, 6, 0, -4),
                                      14,
                                      QColor("#ffffff"));
                addWrappedText(scene,
                               entry.isOverride ? "override" : "base",
                               dispatchRect,
                               14,
                               entry.isOverride ? QColor("#fbbf24") : QColor("#93c5fd"),
                               false);
                ++row;
            }
        } else if (classHasVirtual(layoutInfo)) {
            addSceneText(scene, "vptr exists, but no vtable entries were parsed.",
                         vtablePanel.x() + 18, vtablePanel.y() + 82, 8, QColor("#facc15"));
        }
    } else {
        QRectF noVTablePanel(rightX, canvas.height() * 0.31, rightW, 96);
        addGradientPanel(scene, noVTablePanel, QColor("#0284c7"), QColor("#061c35"), QColor("#08111f"), 10);
        addBoldText(scene,
                    "No Virtual Table",
                    noVTablePanel.x() + 18,
                    noVTablePanel.y() + 14,
                    17,
                    QColor("#e0f2fe"));
        addWrappedText(scene,
                       className + " has no virtual functions, so no vptr or virtual table is shown.",
                       QRectF(noVTablePanel.x() + 18,
                              noVTablePanel.y() + 46,
                              noVTablePanel.width() - 36,
                              40),
                       12,
                       QColor("#93c5fd"));
    }

    if (classHasVirtual(layoutInfo) && !visibleVptrAnchors.isEmpty()) {
        if (!splitVTables) {
            const VisibleVptrAnchor &vptrAnchor = visibleVptrAnchors.first();
            QPointF tableAnchor(vtablePanel.x(), vtablePanel.y() + 145);
            QPainterPath path(vptrAnchor.point);
            path.cubicTo(QPointF(vptrAnchor.point.x() + 80, vptrAnchor.point.y() - 55),
                         QPointF(tableAnchor.x() - 58, tableAnchor.y() + 26),
                         tableAnchor);

            QGraphicsPathItem *wideGlow = scene->addPath(path);
            wideGlow->setPen(QPen(QColor(245, 158, 11, 86), 13, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            wideGlow->setBrush(Qt::NoBrush);

            QGraphicsPathItem *line = scene->addPath(path);
            line->setPen(QPen(QColor("#f59e0b"), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            line->setBrush(Qt::NoBrush);

            QPolygonF arrow;
            arrow << QPointF(tableAnchor.x(), tableAnchor.y())
                  << QPointF(tableAnchor.x() - 16, tableAnchor.y() - 8)
                  << QPointF(tableAnchor.x() - 13, tableAnchor.y() + 11);
            scene->addPolygon(arrow, QPen(QColor("#fbbf24")), QBrush(QColor("#f59e0b")));
        } else {
            for (int i = 0; i < visibleVptrAnchors.size(); ++i) {
                const VisibleVptrAnchor &vptrAnchor = visibleVptrAnchors[i];
                int panelIndex = allVptrLabels.indexOf(vptrAnchor.label);
                if (panelIndex < 0 || panelIndex >= splitPanels.size()) continue;
                QPointF source = vptrAnchor.point;
                QPointF target(splitPanels[panelIndex].x(), splitPanels[panelIndex].center().y());
                QPainterPath path(source);
                path.cubicTo(QPointF(source.x() + 78, source.y()),
                             QPointF(target.x() - 54, target.y()),
                             target);

                QGraphicsPathItem *wideGlow = scene->addPath(path);
                wideGlow->setPen(QPen(QColor(245, 158, 11, 76), 10, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
                wideGlow->setBrush(Qt::NoBrush);

                QGraphicsPathItem *line = scene->addPath(path);
                line->setPen(QPen(QColor("#f59e0b"), 2.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
                line->setBrush(Qt::NoBrush);

                QPolygonF arrow;
                arrow << target
                      << QPointF(target.x() - 15, target.y() - 8)
                      << QPointF(target.x() - 15, target.y() + 8);
                scene->addPolygon(arrow, QPen(QColor("#fbbf24")), QBrush(QColor("#f59e0b")));
            }
        }
    } else if (classHasVirtual(layoutInfo)) {
        const qreal noteY = sizeBadge.bottom() + (vtablePanel.top() - sizeBadge.bottom()) * 0.48 - 14;
        addBoldText(scene,
                    "vptr is on another field page",
                    vtablePanel.x(),
                    noteY,
                    21,
                    QColor("#fbbf24"));
    }

    if (!splitVTables) {
        QRectF infoPanel(rightX, canvas.bottom() - 205, rightW, 100);
        addGradientPanel(scene, infoPanel, QColor("#0284c7"), QColor("#061c35"), QColor("#08111f"), 10);
        addSceneText(scene, "Object:", infoPanel.x() + 18, infoPanel.y() + 12, 15, QColor("#bfdbfe"));
        addBoldText(scene, className, infoPanel.x() + 100, infoPanel.y() + 5, 20, QColor("#22d3ee"));
        addSceneText(scene,
                     QString("Base subobjects: %1").arg(layoutInfo.baseClasses.empty()
                                                            ? QString("none")
                                                            : QString::number(layoutInfo.baseClasses.size())),
                     infoPanel.x() + 18,
                     infoPanel.y() + 43,
                     15,
                     QColor("#bfdbfe"));
        addSceneText(scene,
                     QString("Fields: %1    VTable entries: %2")
                         .arg(layoutInfo.fields.size())
                         .arg(layoutInfo.vtable.size()),
                     infoPanel.x() + 18, infoPanel.y() + 76, 13, QColor("#8fb5d9"));
    }

    QFont legendFont("Helvetica Neue");
    legendFont.setPointSize(14);
    QFontMetrics legendMetrics(legendFont);
    const QStringList legendLabels = {"vptr", "Data Field", "Padding"};
    const qreal swatchSize = 16;
    const qreal swatchTextGap = 10;
    const qreal itemGap = 34;
    const qreal legendPaddingX = 22;
    qreal legendContentW = 0;
    for (int i = 0; i < legendLabels.size(); ++i) {
        if (i > 0) legendContentW += itemGap;
        legendContentW += swatchSize + swatchTextGap + legendMetrics.horizontalAdvance(legendLabels[i]);
    }
    const qreal legendW = legendContentW + legendPaddingX * 2;
    qreal legendX = platform.center().x() - legendW * 0.5;
    qreal legendY = stackY + 20;
    if (legendY + 42 > canvas.bottom() - 16) {
        legendX = platform.center().x() - legendW * 0.5;
        legendY = canvas.bottom() - 58;
    }

    QRectF legend(legendX, legendY, legendW, 42);
    QGraphicsPathItem *legendBg = scene->addPath(roundedRectPath(legend, 12));
    legendBg->setPen(QPen(QColor(14, 165, 233, 70)));
    legendBg->setBrush(QBrush(QColor(2, 8, 23, 185)));

    qreal legendItemX = legend.x() + legendPaddingX;
    QGraphicsPathItem *vptrSwatch = scene->addPath(roundedRectPath(QRectF(legendItemX, legend.y() + 13, swatchSize, swatchSize), 4));
    vptrSwatch->setPen(QPen(QColor("#f59e0b")));
    vptrSwatch->setBrush(QBrush(QColor("#f59e0b")));
    addSceneText(scene, "vptr", legendItemX + swatchSize + swatchTextGap, legend.y() + 4, 14, QColor("#dbeafe"));
    legendItemX += swatchSize + swatchTextGap + legendMetrics.horizontalAdvance("vptr") + itemGap;

    QGraphicsPathItem *fieldSwatch = scene->addPath(roundedRectPath(QRectF(legendItemX, legend.y() + 13, swatchSize, swatchSize), 4));
    fieldSwatch->setPen(QPen(QColor("#0ea5e9")));
    fieldSwatch->setBrush(QBrush(QColor("#0ea5e9")));
    addSceneText(scene, "Data Field", legendItemX + swatchSize + swatchTextGap, legend.y() + 4, 14, QColor("#dbeafe"));
    legendItemX += swatchSize + swatchTextGap + legendMetrics.horizontalAdvance("Data Field") + itemGap;

    QGraphicsPathItem *paddingSwatch = scene->addPath(roundedRectPath(QRectF(legendItemX, legend.y() + 13, swatchSize, swatchSize), 4));
    paddingSwatch->setPen(QPen(QColor("#a855f7")));
    paddingSwatch->setBrush(QBrush(QColor("#a855f7")));
    addSceneText(scene, "Padding", legendItemX + swatchSize + swatchTextGap, legend.y() + 4, 14, QColor("#dbeafe"));

    scene->setSceneRect(canvas);
}

void drawVTable(QGraphicsScene *scene, const ClassLayout &layoutInfo) {
    scene->clear();

    const QRectF canvas(0, 0, 640, 420);
    QLinearGradient background(canvas.topLeft(), canvas.bottomRight());
    background.setColorAt(0.0, QColor("#040716"));
    background.setColorAt(1.0, QColor("#13051f"));
    scene->addRect(canvas, Qt::NoPen, QBrush(background));

    qreal x = 70;
    qreal y = 58;
    const qreal rowWidth = 500;
    const qreal rowHeight = 50;

    QString className = QString::fromStdString(layoutInfo.className);
    addBoldText(scene, "Virtual Function Table", x, 26, 16, QColor("#f8fbff"));
    y += 42;

    if (!classHasVirtual(layoutInfo)) {
        addSceneText(scene,
                     className + " has no virtual function, so no vtable is needed.",
                     x,
                     y,
                     11,
                     QColor("#94a3b8"));
        return;
    }

    int index = 0;
    for (const VTableEntry &entry : layoutInfo.vtable) {
        QRectF rowRect(x, y, rowWidth, rowHeight);
        addGradientPanel(scene,
                         rowRect,
                         entry.isOverride ? QColor("#f59e0b") : QColor("#a855f7"),
                         QColor("#10172a"),
                         QColor("#070a18"),
                         12);

        addBoldText(scene, QString("[%1]").arg(entry.index), x + 18, y + 12, 11, QColor("#f5d0fe"));
        addSceneText(scene,
                     QString::fromStdString(entry.actualClass + "::" + entry.functionName + "()"),
                     x + 82,
                     y + 9,
                     10,
                     QColor("#ffffff"));
        addSceneText(scene,
                     entry.isOverride
                         ? QString("overrides %1").arg(QString::fromStdString(entry.ownerClass))
                         : QString("declared in %1").arg(QString::fromStdString(entry.ownerClass)),
                     x + 82,
                     y + 28,
                     8,
                     entry.isOverride ? QColor("#fbbf24") : QColor("#93c5fd"));

        ++index;
        y += rowHeight + 12;
    }

    if (index == 0) {
        addSceneText(scene,
                     "vptr exists through inheritance, but no local virtual functions were listed.",
                     x,
                     y,
                     10,
                     QColor("#facc15"));
    }

    scene->setSceneRect(canvas);
}

void fillFieldTable(QTableWidget *table,
                    const ClassLayout &layoutInfo,
                    const std::function<void(int)> &explainFieldAt) {
    table->clear();
    table->setColumnCount(7);
    table->setHorizontalHeaderLabels({"Offset", "Size", "Type", "Name", "Owner", "Kind", "Explain"});
    table->setRowCount(static_cast<int>(layoutInfo.fields.size()));

    for (int i = 0; i < static_cast<int>(layoutInfo.fields.size()); ++i) {
        const FieldInfo &field = layoutInfo.fields[i];

        QString kind = "field";
        if (field.isVptr) kind = "vptr";
        if (field.isPadding) kind = "padding";

        table->setItem(i, 0, new QTableWidgetItem(QString::number(field.offset)));
        table->setItem(i, 1, new QTableWidgetItem(QString::number(field.size)));
        table->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(field.typeName)));
        table->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(field.name)));
        QString ownerText = field.ownerPath.empty()
                                ? QString::fromStdString(field.ownerClass)
                                : inheritancePathText(field.ownerPath);
        table->setItem(i, 4, new QTableWidgetItem(ownerText));
        table->setItem(i, 5, new QTableWidgetItem(kind));

        QPushButton *explainButton = new QPushButton("解释", table);
        explainButton->setObjectName("SecondaryButton");
        explainButton->setCursor(Qt::PointingHandCursor);
        explainButton->setStyleSheet(
            "QPushButton { font-size: 11px; padding: 2px 8px; border-radius: 6px; }"
        );
        QObject::connect(explainButton, &QPushButton::clicked, table, [explainFieldAt, i]() {
            explainFieldAt(i);
        });
        table->setCellWidget(i, 6, explainButton);
    }

    table->horizontalHeader()->setStretchLastSection(true);
    table->resizeColumnsToContents();
}

void fillFunctionTable(QTableWidget *table, const ClassLayout &layoutInfo) {
    table->clear();
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({"Signature", "Virtual", "Owner", "VTable Index"});
    table->setRowCount(static_cast<int>(layoutInfo.functions.size()));

    for (int i = 0; i < static_cast<int>(layoutInfo.functions.size()); ++i) {
        const FunctionInfo &fn = layoutInfo.functions[i];

        table->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(fn.signature)));
        table->setItem(i, 1, new QTableWidgetItem(fn.isVirtual ? "true" : "false"));
        table->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(fn.ownerClass)));

        QString indexText = fn.vtableIndex >= 0
                                ? QString::number(fn.vtableIndex)
                                : "-";
        table->setItem(i, 3, new QTableWidgetItem(indexText));
    }

    table->horizontalHeader()->setStretchLastSection(true);
    table->resizeColumnsToContents();
}

void fillBaseTable(QTableWidget *table, const ClassLayout &layoutInfo) {
    table->clear();
    table->setColumnCount(6);
    table->setHorizontalHeaderLabels({"Path", "Base", "Access", "Offset", "Size", "Direct"});
    table->setRowCount(static_cast<int>(layoutInfo.baseClasses.size()));

    for (int i = 0; i < static_cast<int>(layoutInfo.baseClasses.size()); ++i) {
        const BaseClassInfo &base = layoutInfo.baseClasses[i];
        QString path = inheritancePathText(base.path);
        bool isDirect = QString::fromStdString(base.path).count(">") == 1;

        table->setItem(i, 0, new QTableWidgetItem(path));
        table->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(base.name)));
        table->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(base.access)));
        table->setItem(i, 3, new QTableWidgetItem(QString::number(base.offset)));
        table->setItem(i, 4, new QTableWidgetItem(QString::number(base.size)));
        table->setItem(i, 5, new QTableWidgetItem(isDirect ? "yes" : "ancestor"));
    }

    if (layoutInfo.baseClasses.empty()) {
        table->setRowCount(1);
        table->setItem(0, 0, new QTableWidgetItem("none"));
        table->setItem(0, 1, new QTableWidgetItem("-"));
        table->setItem(0, 2, new QTableWidgetItem("-"));
        table->setItem(0, 3, new QTableWidgetItem("-"));
        table->setItem(0, 4, new QTableWidgetItem("-"));
        table->setItem(0, 5, new QTableWidgetItem("-"));
    }

    table->horizontalHeader()->setStretchLastSection(true);
    table->resizeColumnsToContents();
}

void configureDataTable(QTableWidget *table) {
    table->verticalHeader()->setVisible(true);
    table->setFrameShape(QFrame::NoFrame);
    table->setAutoFillBackground(false);
    table->viewport()->setAutoFillBackground(false);
    table->viewport()->setStyleSheet("background-color: #030817; border: none;");
    table->horizontalHeader()->setHighlightSections(false);
    table->verticalHeader()->setHighlightSections(false);

    QPalette headerPalette = table->verticalHeader()->palette();
    headerPalette.setColor(QPalette::Window, QColor("#10233a"));
    headerPalette.setColor(QPalette::Base, QColor("#10233a"));
    QPalette blankHeaderPalette = table->verticalHeader()->viewport()->palette();
    blankHeaderPalette.setColor(QPalette::Window, QColor("#030817"));
    blankHeaderPalette.setColor(QPalette::Base, QColor("#030817"));
    table->verticalHeader()->setAutoFillBackground(true);
    table->verticalHeader()->setPalette(headerPalette);
    table->verticalHeader()->viewport()->setAutoFillBackground(true);
    table->verticalHeader()->viewport()->setPalette(blankHeaderPalette);
    table->verticalHeader()->setStyleSheet(
        "QHeaderView { background-color: #030817; border: none; }"
        "QHeaderView::section { background-color: #10233a; color: #e5e7eb; "
        "border: 1px solid #0b2743; padding: 6px; }"
    );
    table->horizontalHeader()->setStyleSheet(
        "QHeaderView { background-color: #030817; border: none; }"
        "QHeaderView::section { background-color: #10233a; color: #e5e7eb; "
        "border: 1px solid #0b2743; padding: 6px; }"
    );
}

QFrame *makeTableFrame(QTableWidget *table, QWidget *parent) {
    QFrame *frame = new QFrame(parent);
    frame->setObjectName("TableFrame");
    QVBoxLayout *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(3, 3, 3, 3);
    layout->setSpacing(0);
    layout->addWidget(table);
    return frame;
}

QString inheritancePathText(const string &path);
QString baseSummaryText(const ClassLayout &layoutInfo);
int directBaseCount(const ClassLayout &layoutInfo);

QString makeClassSummary(const ClassLayout &layoutInfo) {
    int fieldCount = 0;
    int paddingCount = 0;
    int functionCount = static_cast<int>(layoutInfo.functions.size());
    int virtualCount = 0;

    for (const FieldInfo &field : layoutInfo.fields) {
        if (field.isPadding) {
            ++paddingCount;
        } else if (!field.isVptr) {
            ++fieldCount;
        }
    }

    for (const FunctionInfo &fn : layoutInfo.functions) {
        if (fn.isVirtual) ++virtualCount;
    }

    QString text;
    text += QString("Class: %1\n")
                .arg(QString::fromStdString(layoutInfo.className));

    text += QString("Direct bases: %1\n").arg(directBaseCount(layoutInfo));
    text += QString("Base subobjects: %1\n").arg(baseSummaryText(layoutInfo));

    text += QString("Size: %1 bytes\n").arg(layoutInfo.totalSize);
    text += QString("Fields: %1\n").arg(fieldCount);
    text += QString("Padding blocks: %1\n").arg(paddingCount);
    text += QString("Functions: %1\n").arg(functionCount);
    text += QString("Virtual functions: %1\n").arg(virtualCount);

    return text;
}

QString inheritancePathText(const string &path) {
    return QString::fromStdString(path).replace(">", "::");
}

QString baseSummaryText(const ClassLayout &layoutInfo) {
    if (layoutInfo.baseClasses.empty()) return "none";

    QStringList bases;
    for (const BaseClassInfo &base : layoutInfo.baseClasses) {
        QString path = inheritancePathText(base.path);
        QString access = QString::fromStdString(base.access);
        bases << QString("%1 (%2, offset %3, size %4)")
                     .arg(path, access)
                     .arg(base.offset)
                     .arg(base.size);
    }

    return bases.join("\n      ");
}

int directBaseCount(const ClassLayout &layoutInfo) {
    int count = 0;
    for (const BaseClassInfo &base : layoutInfo.baseClasses) {
        if (QString::fromStdString(base.path).count(">") == 1) ++count;
    }
    return count;
}

const BaseClassInfo *baseInfoForPath(const ClassLayout &layoutInfo,
                                     const string &path) {
    for (const BaseClassInfo &base : layoutInfo.baseClasses) {
        if (base.path == path) return &base;
    }
    return nullptr;
}

int baseNameOccurrenceCount(const ClassLayout &layoutInfo,
                            const string &baseName) {
    int count = 0;
    for (const BaseClassInfo &base : layoutInfo.baseClasses) {
        if (base.name == baseName) ++count;
    }
    return count;
}

QString fieldDisplayName(const FieldInfo &field) {
    if (field.isVptr) return "虚表指针 vptr";
    if (field.isPadding) return "padding 填充";
    return QString("%1 %2")
        .arg(QString::fromStdString(field.typeName))
        .arg(QString::fromStdString(field.name));
}

QString explainField(const ClassLayout &layoutInfo, const FieldInfo &field) {
    QString text;
    text += QString("当前类：%1\n").arg(QString::fromStdString(layoutInfo.className));
    text += QString("字段：%1\n\n").arg(fieldDisplayName(field));

    if (field.isVptr) {
        text += QString("这是虚表指针，位于对象偏移 %1，大小 %2 字节。\n")
                    .arg(field.offset)
                    .arg(field.size);
        if (!field.ownerPath.empty()) {
            text += QString("它关联的子对象路径是 %1。\n")
                        .arg(inheritancePathText(field.ownerPath));
            const BaseClassInfo *base = baseInfoForPath(layoutInfo, field.ownerPath);
            if (base) {
                text += QString("这个父类子对象从 offset %1 开始，大小 %2 字节；该 vptr 位于这个子对象的开头。\n")
                            .arg(base->offset)
                            .arg(base->size);
            }
        }
        if (layoutInfo.baseClasses.size() > 1) {
            text += "在普通多继承里，如果多个父类子对象都需要支持 virtual 函数，派生类对象中可能出现多个 vptr。\n";
        }
        text += "只要类本身或父类含有 virtual 函数，对象里通常会放一个隐藏指针，指向虚函数表。\n";
        text += "调用 virtual 函数时，程序会通过这个指针找到当前实际类型应该执行的函数版本。\n";
    } else if (field.isPadding) {
        text += QString("这是编译器插入的填充字节，起始偏移 %1，长度 %2 字节。\n")
                    .arg(field.offset)
                    .arg(field.size);
        text += "它不属于任何成员变量，主要用于让后面的字段或整个对象满足对齐要求。\n";
        text += "如果这段填充出现在最后，它通常是尾部填充，用来保证数组里的下一个对象也能从合适地址开始。\n";
    } else {
        QString owner = QString::fromStdString(field.ownerClass);
        QString typeName = QString::fromStdString(field.typeName);
        text += QString("这是一个普通成员变量，类型是 %1，大小 %2 字节，起始偏移 %3。\n")
                    .arg(typeName)
                    .arg(field.size)
                    .arg(field.offset);
        QString ownerPath = field.ownerPath.empty()
                                ? owner
                                : inheritancePathText(field.ownerPath);
        if (!owner.isEmpty() && owner != QString::fromStdString(layoutInfo.className)) {
            text += QString("它由继承路径 %1 引入；多继承或非虚菱形继承时，同一个祖先类可能沿不同路径出现多份子对象。\n")
                        .arg(ownerPath);
            const BaseClassInfo *base = baseInfoForPath(layoutInfo, field.ownerPath);
            if (base) {
                text += QString("这条路径对应的父类子对象从 offset %1 开始，大小 %2 字节，所以字段最终 offset 是在整个 %3 对象中实测得到的。\n")
                            .arg(base->offset)
                            .arg(base->size)
                            .arg(QString::fromStdString(layoutInfo.className));
            }
            if (baseNameOccurrenceCount(layoutInfo, field.ownerClass) > 1) {
                text += QString("注意：%1 在当前对象中出现了多份父类子对象，这通常来自非虚菱形继承；表格按路径区分它们，不是重复显示错误。\n")
                            .arg(owner);
            }
        } else {
            text += QString("它由当前类 %1 声明。\n").arg(QString::fromStdString(layoutInfo.className));
        }
        text += "这里的 offset 是字段地址减去对象起始地址得到的，更接近真实对象在内存中的位置。\n";
    }

    text += "\n学习提示：\n";
    text += "1. 对象的总大小包含真实字段、虚表指针和必要 padding。\n";
    text += "2. static 成员不存放在每个对象里，所以不会出现在对象布局中。\n";
    text += "3. 字段大小来自 sizeof(obj.field)，对象大小来自 sizeof(Class)。\n";
    return text;
}

void showTextDialog(QWidget *parent, const QString &title, const QString &text) {
    QDialog *dialog = new QDialog(parent);
    dialog->setWindowTitle(title);
    dialog->resize(560, 420);

    QVBoxLayout *layout = new QVBoxLayout(dialog);
    QTextEdit *viewer = new QTextEdit(dialog);
    viewer->setReadOnly(true);
    viewer->setPlainText(text);

    QPushButton *closeButton = new QPushButton("关闭", dialog);
    closeButton->setObjectName("SecondaryButton");
    QObject::connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    layout->addWidget(viewer);
    layout->addLayout(buttonLayout);
    dialog->show();
}

QString layoutContext(const QVector<ClassLayout> &layouts) {
    QString text;
    for (const ClassLayout &layoutInfo : layouts) {
        text += QString("Class %1, size %2 bytes")
                    .arg(QString::fromStdString(layoutInfo.className))
                    .arg(layoutInfo.totalSize);
        if (!layoutInfo.baseClasses.empty()) {
            text += QString(", base subobjects %1").arg(layoutInfo.baseClasses.size());
        }
        if (!layoutInfo.baseClasses.empty()) {
            text += "\nBase subobjects:\n";
            for (const BaseClassInfo &base : layoutInfo.baseClasses) {
                text += QString("  path=%1 base=%2 access=%3 offset=%4 size=%5 direct=%6\n")
                            .arg(inheritancePathText(base.path))
                            .arg(QString::fromStdString(base.name))
                            .arg(QString::fromStdString(base.access))
                            .arg(base.offset)
                            .arg(base.size)
                            .arg(QString::fromStdString(base.path).count(">") == 1 ? "true" : "false");
            }
        }
        text += "\nFields:\n";
        for (const FieldInfo &field : layoutInfo.fields) {
            QString kind = "field";
            if (field.isVptr) kind = "vptr";
            if (field.isPadding) kind = "padding";
            text += QString("  offset=%1 size=%2 type=%3 name=%4 owner=%5 kind=%6\n")
                        .arg(field.offset)
                        .arg(field.size)
                        .arg(QString::fromStdString(field.typeName))
                        .arg(QString::fromStdString(field.name))
                        .arg(field.ownerPath.empty()
                                 ? QString::fromStdString(field.ownerClass)
                                 : inheritancePathText(field.ownerPath))
                        .arg(kind);
        }
        if (!layoutInfo.vtable.empty()) {
            text += "VTable:\n";
            for (const VTableEntry &entry : layoutInfo.vtable) {
                text += QString("  [%1] %2::%3 owner=%4 override=%5\n")
                            .arg(entry.index)
                            .arg(QString::fromStdString(entry.actualClass))
                            .arg(QString::fromStdString(entry.functionName))
                            .arg(QString::fromStdString(entry.ownerClass))
                            .arg(entry.isOverride ? "true" : "false");
            }
        }
        text += "\n";
    }
    return text;
}

QString buildAiPrompt(const QString &code,
                      const QVector<ClassLayout> &layouts,
                      int selectedRow) {
    QString selectedClass = "none";
    if (selectedRow >= 0 && selectedRow < layouts.size()) {
        selectedClass = QString::fromStdString(layouts[selectedRow].className);
    }

    return QString(
        "请面向正在学习 C++ 对象内存布局的同学讲解下面代码。\n"
        "要求：先用 3-5 条说明最重要的布局结论，再指出 padding/vptr/继承字段如何影响大小，"
        "如果出现多继承，要解释父类子对象 base subobject、继承路径和多个 vptr 的原因；"
        "最后给出 2 个可操作的学习建议。不要逐字段复述表格内容，单字段细节由字段解释按钮负责；"
        "你要讲 C++ 对象布局知识、推理过程、易错点和观察方法，不要宣传工具本身。"
        "不要泛泛而谈，要引用关键 offset 和 size。必须说明 offset/size 以当前编译器实测结果为准，"
        "你的讲解是教学性解释，不是 C++ 标准层面的绝对保证。\n\n"
        "当前选中类：%1\n\n"
        "用户代码：\n%2\n\n"
        "解析结果：\n%3")
        .arg(selectedClass, code, layoutContext(layouts));
}

QString defaultModelForProvider(const QString &provider) {
    if (provider == "OpenAI") return "gpt-4.1-mini";
    if (provider == "DeepSeek") return "deepseek-chat";
    if (provider == "通义千问") return "qwen-plus";
    if (provider == "Kimi") return "moonshot-v1-8k";
    if (provider == "Ollama 本地") return "qwen2.5:7b";
    return "";
}

QString envKeyForProvider(const QString &provider) {
    if (provider == "OpenAI") return "OPENAI_API_KEY";
    if (provider == "DeepSeek") return "DEEPSEEK_API_KEY";
    if (provider == "通义千问") return "DASHSCOPE_API_KEY";
    if (provider == "Kimi") return "MOONSHOT_API_KEY";
    return "";
}

QUrl endpointForProvider(const QString &provider) {
    if (provider == "OpenAI") return QUrl("https://api.openai.com/v1/chat/completions");
    if (provider == "DeepSeek") return QUrl("https://api.deepseek.com/chat/completions");
    if (provider == "通义千问") return QUrl("https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions");
    if (provider == "Kimi") return QUrl("https://api.moonshot.cn/v1/chat/completions");
    if (provider == "Ollama 本地") return QUrl("http://localhost:11434/api/chat");
    return {};
}

AiRequestConfig buildAiRequestConfig(const QString &provider,
                                     const QString &model,
                                     const QString &apiKey,
                                     const QString &prompt) {
    AiRequestConfig config;
    config.provider = provider;
    config.model = model.trimmed().isEmpty() ? defaultModelForProvider(provider) : model.trimmed();
    config.url = endpointForProvider(provider);
    config.apiKey = apiKey.trimmed().toUtf8();
    config.needsApiKey = provider != "Ollama 本地";

    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] =
        "你是程序设计实习课程的 C++ 助教，擅长用准确但容易理解的语言讲解对象内存布局。";

    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = prompt;

    QJsonArray messages;
    messages.append(systemMessage);
    messages.append(userMessage);

    config.body["model"] = config.model;
    config.body["messages"] = messages;

    if (provider == "Ollama 本地") {
        config.body["stream"] = false;
    } else {
        config.body["max_tokens"] = 900;
    }

    return config;
}

QString localTutorText(const QString &code,
                       const QVector<ClassLayout> &layouts,
                       int selectedRow) {
    Q_UNUSED(code);
    QString text;
    text += "AI 助教讲解（本地版）\n\n";
    text += "提示：讲解基于当前编译器实测布局生成，适合辅助理解；具体 offset/size 以表格中的实测结果为准。\n\n";
    text += "这部分偏向整体讲解；单个字段的 offset 和 size 可以点成员变量表里的“解释”按钮查看。\n\n";

    if (selectedRow >= 0 && selectedRow < layouts.size()) {
        const ClassLayout &layoutInfo = layouts[selectedRow];

        int normalFields = 0;
        int inheritedFields = 0;
        int paddingBlocks = 0;
        int paddingBytes = 0;
        int vptrCount = 0;
        int virtualFunctions = 0;
        int overrideCount = 0;
        int ownFields = 0;
        int largestFieldSize = -1;
        QString largestFieldName;
        QString firstPaddingCause;
        int directBases = directBaseCount(layoutInfo);
        QStringList directBaseDescriptions;
        QStringList repeatedBaseDescriptions;

        for (int i = 0; i < static_cast<int>(layoutInfo.fields.size()); ++i) {
            const FieldInfo &field = layoutInfo.fields[i];
            if (field.isVptr) {
                ++vptrCount;
            } else if (field.isPadding) {
                ++paddingBlocks;
                paddingBytes += field.size;
                if (firstPaddingCause.isEmpty()) {
                    for (int j = i + 1; j < static_cast<int>(layoutInfo.fields.size()); ++j) {
                        const FieldInfo &next = layoutInfo.fields[j];
                        if (!next.isPadding && !next.isVptr) {
                            firstPaddingCause = QString("偏移 %1 的 %2 字节 padding 后面接着字段 %3，通常是为了让这个字段从更合适的地址开始。")
                                                    .arg(field.offset)
                                                    .arg(field.size)
                                                    .arg(QString::fromStdString(next.name));
                            break;
                        }
                    }
                    if (firstPaddingCause.isEmpty()) {
                        firstPaddingCause = QString("末尾 %1 字节 padding 主要是为了保证数组中下一个对象的起始地址也满足对齐要求。")
                                                .arg(field.size);
                    }
                }
            } else {
                ++normalFields;
                if (!field.ownerClass.empty()
                    && field.ownerClass != layoutInfo.className) {
                    ++inheritedFields;
                } else {
                    ++ownFields;
                }
                if (field.size > largestFieldSize) {
                    largestFieldSize = field.size;
                    largestFieldName = QString::fromStdString(field.name);
                }
            }
        }

        for (const FunctionInfo &fn : layoutInfo.functions) {
            if (fn.isVirtual) ++virtualFunctions;
        }
        for (const VTableEntry &entry : layoutInfo.vtable) {
            if (entry.isOverride) ++overrideCount;
        }

        QMap<QString, int> baseNameCounts;
        for (const BaseClassInfo &base : layoutInfo.baseClasses) {
            QString baseName = QString::fromStdString(base.name);
            baseNameCounts[baseName] = baseNameCounts.value(baseName) + 1;
            if (QString::fromStdString(base.path).count(">") == 1) {
                directBaseDescriptions << QString("%1(offset %2, size %3)")
                                              .arg(inheritancePathText(base.path))
                                              .arg(base.offset)
                                              .arg(base.size);
            }
        }
        for (auto it = baseNameCounts.constBegin(); it != baseNameCounts.constEnd(); ++it) {
            if (it.value() > 1) {
                repeatedBaseDescriptions << QString("%1 出现 %2 份")
                                                .arg(it.key())
                                                .arg(it.value());
            }
        }

        const double paddingRatio = layoutInfo.totalSize > 0
                                        ? static_cast<double>(paddingBytes) / layoutInfo.totalSize
                                        : 0.0;

        text += "一、这个样例最值得先看什么\n";
        if (layouts.size() > 1) {
            text += "- 这里有多个类，可以对比对象大小如何沿着继承链变化：";
            for (int i = 0; i < layouts.size(); ++i) {
                const ClassLayout &item = layouts[i];
                text += QString("%1%2(%3B)")
                            .arg(i == 0 ? " " : " -> ")
                            .arg(QString::fromStdString(item.className))
                            .arg(item.totalSize);
            }
            text += "。\n";
        } else {
            text += "- 这里重点不是继承链，而是单个类内部字段顺序、对齐和 padding 如何共同决定 sizeof。\n";
        }
        if (vptrCount > 0) {
            text += QString("- 这个样例涉及虚函数：对象里检测到 %1 个 vptr，虚表里有 %2 个入口，其中 %3 个表现为覆盖/重写。\n")
                        .arg(vptrCount)
                        .arg(layoutInfo.vtable.size())
                        .arg(overrideCount);
        }
        if (directBases > 1) {
            text += QString("- 这个样例涉及普通多继承：选中类有 %1 个直接父类子对象，分别是 %2。\n")
                        .arg(directBases)
                        .arg(directBaseDescriptions.join("；"));
        }
        if (!repeatedBaseDescriptions.isEmpty()) {
            text += QString("- 这里出现了非虚菱形继承特征：%1。每一份祖先子对象都有独立路径和独立字段位置。\n")
                        .arg(repeatedBaseDescriptions.join("；"));
        }
        if (paddingBlocks > 0) {
            text += QString("- padding 是这个样例的一个重点：%1 段、共 %2 字节，占选中类大小约 %3%。\n")
                        .arg(paddingBlocks)
                        .arg(paddingBytes)
                        .arg(static_cast<int>(paddingRatio * 100.0 + 0.5));
        } else {
            text += "- 这个类没有检测到 padding，说明当前字段排列在这个编译环境下比较紧凑。\n";
        }

        text += QString("\n二、选中类 %1 的布局重点\n")
                    .arg(QString::fromStdString(layoutInfo.className))
                    ;
        text += QString("- 总大小是 %1 字节，其中普通成员变量 %2 个：自己声明 %3 个，继承来的字段 %4 个。\n")
                    .arg(layoutInfo.totalSize)
                    .arg(normalFields)
                    .arg(ownFields)
                    .arg(inheritedFields);
        if (directBases > 1) {
            text += QString("- 它不是只有一个父类前缀，而是由多个父类子对象加上自己的成员组成：%1。\n")
                        .arg(directBaseDescriptions.join("；"));
        } else if (!layoutInfo.parentClass.empty()) {
            text += QString("- 因为继承自 %1，父类对象部分会先放进当前对象，子类新增字段排在后面。\n")
                        .arg(QString::fromStdString(layoutInfo.parentClass));
        }
        if (vptrCount > 0) {
            text += QString("- 这个对象含有 %1 个 vptr，说明虚函数机制会占用对象内存；虚函数数量是 %2。\n")
                        .arg(vptrCount)
                        .arg(virtualFunctions);
            if (directBases > 1 && vptrCount > 1) {
                text += "- 多继承下出现多个 vptr 通常是因为不同父类子对象都需要从自己的视角支持 virtual 函数调用。\n";
            }
        } else {
            text += "- 当前选中类没有检测到 vptr，对象大小主要由普通成员和 padding 决定。\n";
        }
        text += QString("- padding 有 %1 段，总计约 %2 字节。它不是成员变量，而是编译器为了对齐插入的空隙。\n")
                    .arg(paddingBlocks)
                    .arg(paddingBytes);
        if (!largestFieldName.isEmpty()) {
            text += QString("- 选中类里占空间最大的普通字段是 %1，大小 %2 字节；大字段经常会影响后续字段的对齐位置。\n")
                        .arg(largestFieldName)
                        .arg(largestFieldSize);
        }

        text += "\n三、结合这个样例的知识点\n";
        text += "- offset 的含义是“成员地址 - 对象起始地址”，所以它反映的是对象内部真实位置，不是声明中的第几个字段。\n";
        if (!firstPaddingCause.isEmpty()) {
            text += QString("- %1\n").arg(firstPaddingCause);
        }
        if (!repeatedBaseDescriptions.isEmpty()) {
            text += "- 非虚菱形继承不会共享祖先子对象；例如两条继承路径到同一个祖先类时，对象中会有两份祖先字段。\n";
        }
        if (layoutInfo.parentClass.empty() && directBases == 0) {
            text += "- 没有父类时，对象布局主要由本类字段、vptr 和 padding 决定。\n";
        } else if (directBases > 1) {
            text += "- 有多个直接父类时，编译器会把各个父类子对象依次放入派生类对象，再安排派生类自己的新增字段。\n";
        } else {
            text += "- 有父类时，子类对象开头通常先是一整个父类子对象；这就是为什么继承来的字段也会占用子类对象空间。\n";
        }
        if (vptrCount > 0) {
            text += "- vptr 让虚函数调用能在运行时根据实际类型分派；它通常出现在对象开头，也会计入 sizeof。\n";
        }
        if (paddingBlocks == 0) {
            text += "- 没有 padding 不代表没有对齐规则，而是这些字段刚好没有制造可见空洞。\n";
        } else if (paddingRatio >= 0.20) {
            text += "- padding 占比偏高时，可以重点怀疑字段顺序是否让小字段夹在大对齐字段前面。\n";
        } else {
            text += "- padding 占比不高时，它更多是在满足基本对齐要求，不一定值得强行优化。\n";
        }

        text += "\n四、建议你尝试的实验\n";
        if (paddingBlocks > 0) {
            text += "- 先调换几个普通字段的声明顺序，再分析一次，观察 padding 段数和总字节数是否减少。\n";
        } else {
            text += "- 可以故意加入一个 bool/char 和一个 double，观察编译器什么时候开始插入 padding。\n";
        }
        if (vptrCount > 0) {
            text += "- 临时去掉 virtual 关键字再分析一次，对比 vptr、虚表和 sizeof 的变化。\n";
        } else {
            text += "- 给类加一个 virtual 函数再分析一次，观察对象里是否新增 vptr。\n";
        }
        if (directBases > 1) {
            text += "- 打开“父类子对象”表格，先看每个 base subobject 的 offset/size，再回到成员变量表理解字段路径。\n";
        } else if (!layoutInfo.parentClass.empty()) {
            text += "- 分别选择父类和子类，比较父类子对象在子类布局中的位置。\n";
        } else if (layouts.size() == 1) {
            text += "- 再写一个继承它的子类，观察父类字段如何被带入子类对象。\n";
        }
    }

    text += "\n想启用真正的大模型讲解，请在上方输入 API Key 后点击“联网讲解”。";
    return text;
}

QString extractOpenAIText(const QJsonObject &root) {
    QString direct = root.value("output_text").toString();
    if (!direct.isEmpty()) return direct;

    const QJsonArray choices = root.value("choices").toArray();
    for (const QJsonValue &choiceValue : choices) {
        const QJsonObject choice = choiceValue.toObject();
        QString text = choice.value("message").toObject().value("content").toString();
        if (!text.isEmpty()) return text;
        text = choice.value("text").toString();
        if (!text.isEmpty()) return text;
    }

    QString ollamaText = root.value("message").toObject().value("content").toString();
    if (!ollamaText.isEmpty()) return ollamaText;

    const QJsonArray output = root.value("output").toArray();
    for (const QJsonValue &outputValue : output) {
        const QJsonObject outputObject = outputValue.toObject();
        const QJsonArray content = outputObject.value("content").toArray();
        for (const QJsonValue &contentValue : content) {
            const QJsonObject contentObject = contentValue.toObject();
            QString text = contentObject.value("text").toString();
            if (!text.isEmpty()) return text;
        }
    }

    return {};
}

QString markdownEscape(QString text) {
    text.replace("\\", "\\\\");
    text.replace("|", "\\|");
    text.replace("\n", " ");
    return text;
}

void showExportSuccessDialog(QWidget *parent) {
    QDialog successDialog(parent);
    successDialog.setWindowTitle("导出报告");
    successDialog.setFixedSize(260, 165);

    QVBoxLayout *successLayout = new QVBoxLayout(&successDialog);
    successLayout->setContentsMargins(18, 12, 18, 16);
    successLayout->setSpacing(0);

    QLabel *successLabel = new QLabel("报告已导出", &successDialog);
    successLabel->setAlignment(Qt::AlignCenter);
    QFont successFont = successLabel->font();
    successFont.setPointSize(20);
    successFont.setBold(true);
    successLabel->setFont(successFont);
    successLabel->setStyleSheet("font-size: 20px; font-weight: 700; color: #dbeafe;");
    successLabel->setMinimumHeight(56);

    QPushButton *okButton = new QPushButton("OK", &successDialog);
    okButton->setFixedSize(84, 44);
    okButton->setObjectName("SecondaryButton");
    QHBoxLayout *buttonRow = new QHBoxLayout();
    buttonRow->addStretch();
    buttonRow->addWidget(okButton);
    buttonRow->addStretch();

    successLayout->addSpacing(4);
    successLayout->addWidget(successLabel);
    successLayout->addSpacing(20);
    successLayout->addLayout(buttonRow);
    successLayout->addStretch();

    QObject::connect(okButton, &QPushButton::clicked, &successDialog, &QDialog::accept);
    successDialog.exec();
}

struct ExampleSnippet {
    QString title;
    QString goal;
    QString code;
};

QVector<ExampleSnippet> exampleSnippets() {
    return {
        {
            "对齐 + 重排 + static",
            "观察 padding、字段顺序优化、static 不进入对象布局，以及 using 类型别名。",
            R"(using Score = double;

class BadLayout {
public:
    char tag;
    Score score;
    bool ready;
    int count;
    static int sharedCount;
};

class BetterLayout {
public:
    char tag;
    bool ready;
    char level;
    int count;
    Score score;
};)"
        },
        {
            "virtual + 单继承",
            "观察单继承下的 vptr、虚函数表、函数覆盖，以及派生类新增字段。",
            R"(class Shape {
public:
    virtual double area() { return 0.0; }
    virtual int type() { return 0; }
    int color;
};

class Circle : public Shape {
public:
    double area() { return 3.14 * radius * radius; }
    int type() { return 1; }
    double radius;
    char tag;
};)"
        },
        {
            "多继承 + 多个 vptr",
            "观察多个父类子对象、多个 vptr、多个虚函数表，以及派生类自己的字段。",
            R"(class Left {
public:
    virtual void leftFunc() {}
    int leftValue;
};

class Right {
public:
    virtual void rightFunc() {}
    int rightValue;
};

class Both : public Left, public Right {
public:
    char tag;
    int ownValue;
};)"
        },
        {
            "非虚菱形 + 父类子对象",
            "观察非虚菱形继承中祖先类子对象出现两份，同时查看每个父类子对象 offset。",
            R"(class A {
public:
    int a;
};

class B : public A {
public:
    int b;
};

class C : public A {
public:
    int c;
};

class D : public B, public C {
public:
    int d;
};)"
        },
        {
            "错误诊断 + include 建议",
            "观察常见标准库类型缺少 include 时，工具如何给出修复建议。",
            R"(class UsesVector {
public:
    std::vector<int> values;
    std::string name;
    int count;
};)"
        }
    };
}

QString defaultExampleCode() {
    const QVector<ExampleSnippet> examples = exampleSnippets();
    for (const ExampleSnippet &example : examples) {
        if (example.title.contains("virtual")) return example.code;
    }
    return examples.isEmpty() ? QString() : examples.first().code;
}

int fieldAlignmentGuess(int size) {
    if (size >= 8) return 8;
    if (size >= 4) return 4;
    if (size >= 2) return 2;
    return 1;
}

int alignUpValue(int value, int align) {
    if (align <= 1) return value;
    int rem = value % align;
    return rem == 0 ? value : value + (align - rem);
}

QString paddingOptimizationText(const ClassLayout &layoutInfo) {
    QVector<FieldInfo> ownFields;
    int paddingBytes = 0;
    int paddingBlocks = 0;
    for (const FieldInfo &field : layoutInfo.fields) {
        if (field.isPadding) {
            ++paddingBlocks;
            paddingBytes += field.size;
            continue;
        }
        if (!field.isVptr &&
            field.ownerClass == layoutInfo.className &&
            field.ownerPath.empty()) {
            ownFields.append(field);
        }
    }

    QString className = QString::fromStdString(layoutInfo.className);
    QString text;
    text += QString("Class: %1\n").arg(className);
    text += QString("Current sizeof: %1 bytes\n").arg(layoutInfo.totalSize);
    text += QString("Current padding: %1 blocks, %2 bytes\n\n").arg(paddingBlocks).arg(paddingBytes);

    if (ownFields.size() < 2) {
        text += "字段顺序优化建议：当前类可调整的数据成员少于 2 个，暂时没有有意义的重排建议。\n";
        text += "说明：继承来的字段、父类子对象和 vptr 不能通过重排当前类成员来改变。\n";
        return text;
    }

    int ownStart = ownFields.first().offset;
    int ownEnd = ownFields.first().offset + ownFields.first().size;
    for (const FieldInfo &field : ownFields) {
        ownStart = std::min(ownStart, field.offset);
        ownEnd = std::max(ownEnd, field.offset + field.size);
    }

    QVector<FieldInfo> suggested = ownFields;
    std::sort(suggested.begin(), suggested.end(), [](const FieldInfo &a, const FieldInfo &b) {
        int alignA = fieldAlignmentGuess(a.size);
        int alignB = fieldAlignmentGuess(b.size);
        if (alignA != alignB) return alignA > alignB;
        if (a.size != b.size) return a.size > b.size;
        return a.name < b.name;
    });

    int estimatedOffset = 0;
    int maxAlign = 1;
    for (const FieldInfo &field : suggested) {
        int align = fieldAlignmentGuess(field.size);
        maxAlign = std::max(maxAlign, align);
        estimatedOffset = alignUpValue(estimatedOffset, align);
        estimatedOffset += field.size;
    }
    int estimatedSpan = alignUpValue(estimatedOffset, maxAlign);
    int currentOwnSpan = ownEnd - ownStart;
    int possibleSaving = std::max(0, currentOwnSpan - estimatedSpan);

    text += "当前类自己声明的数据成员顺序：\n";
    for (const FieldInfo &field : ownFields) {
        text += QString("- %1, offset %2, size %3\n")
                    .arg(fieldDisplayName(field))
                    .arg(field.offset)
                    .arg(field.size);
    }

    text += "\n建议尝试的字段顺序：\n";
    for (const FieldInfo &field : suggested) {
        text += QString("- %1, size %2, estimated align %3\n")
                    .arg(fieldDisplayName(field))
                    .arg(field.size)
                    .arg(fieldAlignmentGuess(field.size));
    }

    text += QString("\n估算：当前类自身字段覆盖区间约 %1 bytes，按建议顺序估算约 %2 bytes，理论上最多可能减少约 %3 bytes。\n")
                .arg(currentOwnSpan)
                .arg(estimatedSpan)
                .arg(possibleSaving);
    text += "注意：这是基于当前实测字段 size 的教学性估算，不会自动修改代码；最终 offset / sizeof 仍应以重新分析后的 clang++ 探针实测结果为准。\n";
    return text;
}

QString makeReport(const QString &code, const QVector<ClassLayout> &layouts) {
    QString result;
    result += "# C++ Memory Visualizer 分析报告\n\n";
    result += QString("- 生成时间：%1\n")
                  .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    result += "- 说明：offset / size / sizeof 均来自当前 clang++ 探针实测结果；vptr 和虚表信息用于教学解释，不代表 C++ 标准规定的绝对布局。\n\n";

    result += "## 原始代码\n\n";
    result += "```cpp\n";
    result += code.trimmed();
    result += "\n```\n\n";

    result += "## 类列表\n\n";
    result += "| 类名 | sizeof | 直接父类数 | 父类子对象数 | vptr 数 | padding 字节 |\n";
    result += "|---|---:|---:|---:|---:|---:|\n";
    for (const ClassLayout &layoutInfo : layouts) {
        int vptrCount = 0;
        int paddingBytes = 0;
        for (const FieldInfo &field : layoutInfo.fields) {
            if (field.isVptr) ++vptrCount;
            if (field.isPadding) paddingBytes += field.size;
        }

        result += QString("| %1 | %2 | %3 | %4 | %5 | %6 |\n")
                      .arg(markdownEscape(QString::fromStdString(layoutInfo.className)))
                      .arg(layoutInfo.totalSize)
                      .arg(directBaseCount(layoutInfo))
                      .arg(layoutInfo.baseClasses.size())
                      .arg(vptrCount)
                      .arg(paddingBytes);
    }

    for (const ClassLayout &layoutInfo : layouts) {
        result += QString("\n## %1\n\n")
                      .arg(markdownEscape(QString::fromStdString(layoutInfo.className)));
        result += QString("- sizeof：%1 bytes\n")
                      .arg(layoutInfo.totalSize);
        result += QString("- 直接父类数：%1\n")
                      .arg(directBaseCount(layoutInfo));
        result += QString("- 父类子对象数：%1\n\n")
                      .arg(layoutInfo.baseClasses.size());

        if (!layoutInfo.baseClasses.empty()) {
            result += "### 父类子对象\n\n";
            result += "| Path | Base | Access | Offset | Size | Direct |\n";
            result += "|---|---|---|---:|---:|---|\n";
            for (const BaseClassInfo &base : layoutInfo.baseClasses) {
                QString path = inheritancePathText(base.path);
                result += QString("| %1 | %2 | %3 | %4 | %5 | %6 |\n")
                              .arg(markdownEscape(path))
                              .arg(markdownEscape(QString::fromStdString(base.name)))
                              .arg(markdownEscape(QString::fromStdString(base.access)))
                              .arg(base.offset)
                              .arg(base.size)
                              .arg(QString::fromStdString(base.path).count(">") == 1 ? "yes" : "ancestor");
            }
            result += "\n";
        }

        result += "### 成员变量布局\n\n";
        result += "| Offset | Size | Type | Name | Owner / Path | Kind |\n";
        result += "|---:|---:|---|---|---|---|\n";
        for (const FieldInfo &field : layoutInfo.fields) {
            QString kind = "field";
            if (field.isVptr) kind = "vptr";
            if (field.isPadding) kind = "padding";
            QString owner = field.ownerPath.empty()
                                ? QString::fromStdString(field.ownerClass)
                                : inheritancePathText(field.ownerPath);
            result += QString("| %1 | %2 | %3 | %4 | %5 | %6 |\n")
                          .arg(field.offset)
                          .arg(field.size)
                          .arg(markdownEscape(QString::fromStdString(field.typeName)))
                          .arg(markdownEscape(QString::fromStdString(field.name)))
                          .arg(markdownEscape(owner))
                          .arg(kind);
        }

        int paddingBlocks = 0;
        int paddingBytes = 0;
        for (const FieldInfo &field : layoutInfo.fields) {
            if (field.isPadding) {
                ++paddingBlocks;
                paddingBytes += field.size;
            }
        }
        result += QString("\nPadding 汇总：%1 段，共 %2 bytes。\n\n")
                      .arg(paddingBlocks)
                      .arg(paddingBytes);

        result += "### 字段顺序优化建议\n\n";
        result += "```text\n";
        result += paddingOptimizationText(layoutInfo).trimmed();
        result += "\n```\n\n";

        result += "### 成员函数\n\n";
        if (layoutInfo.functions.empty()) {
            result += "未解析到成员函数。\n\n";
        } else {
            result += "| Signature | Virtual | Owner | VTable Index |\n";
            result += "|---|---|---|---:|\n";
        }
        for (const FunctionInfo &fn : layoutInfo.functions) {
            result += QString("| %1 | %2 | %3 | %4 |\n")
                          .arg(markdownEscape(QString::fromStdString(fn.signature)))
                          .arg(fn.isVirtual ? "yes" : "no")
                          .arg(markdownEscape(QString::fromStdString(fn.ownerClass)))
                          .arg(fn.vtableIndex >= 0 ? QString::number(fn.vtableIndex) : "-");
        }
        result += "\n";

        result += "### Virtual Table / Dispatch View\n\n";
        if (layoutInfo.vtable.empty()) {
            result += "未解析到虚函数表条目。\n\n";
        } else {
            result += "| Index | Function | Owner | Actual | Dispatch |\n";
            result += "|---:|---|---|---|---|\n";
            for (const VTableEntry &entry : layoutInfo.vtable) {
                result += QString("| %1 | %2 | %3 | %4 | %5 |\n")
                              .arg(entry.index)
                              .arg(markdownEscape(QString::fromStdString(entry.functionName + "()")))
                              .arg(markdownEscape(QString::fromStdString(entry.ownerClass)))
                              .arg(markdownEscape(QString::fromStdString(entry.actualClass)))
                              .arg(entry.isOverride ? "override" : "base");
            }
            result += "\n";
        }
    }

    return result;
}

QString removeCommentsAndStringLiteralsForCheck(const QString &code) {
    QString out;
    out.reserve(code.size());

    bool inLineComment = false;
    bool inBlockComment = false;
    bool inString = false;
    bool inChar = false;
    bool escaped = false;

    for (int i = 0; i < code.size(); ++i) {
        QChar ch = code[i];
        QChar next = (i + 1 < code.size()) ? code[i + 1] : QChar();

        if (inLineComment) {
            if (ch == '\n') {
                inLineComment = false;
                out += '\n';
            } else {
                out += ' ';
            }
            continue;
        }

        if (inBlockComment) {
            if (ch == '*' && next == '/') {
                inBlockComment = false;
                out += "  ";
                ++i;
            } else {
                out += (ch == '\n') ? '\n' : ' ';
            }
            continue;
        }

        if (inString) {
            if (escaped) {
                escaped = false;
                out += ' ';
                continue;
            }
            if (ch == '\\') {
                escaped = true;
                out += ' ';
                continue;
            }
            if (ch == '"') {
                inString = false;
            }
            out += (ch == '\n') ? '\n' : ' ';
            continue;
        }

        if (inChar) {
            if (escaped) {
                escaped = false;
                out += ' ';
                continue;
            }
            if (ch == '\\') {
                escaped = true;
                out += ' ';
                continue;
            }
            if (ch == '\'') {
                inChar = false;
            }
            out += (ch == '\n') ? '\n' : ' ';
            continue;
        }

        if (ch == '/' && next == '/') {
            inLineComment = true;
            out += "  ";
            ++i;
            continue;
        }

        if (ch == '/' && next == '*') {
            inBlockComment = true;
            out += "  ";
            ++i;
            continue;
        }

        if (ch == '"') {
            inString = true;
            out += ' ';
            continue;
        }

        if (ch == '\'') {
            inChar = true;
            out += ' ';
            continue;
        }

        out += ch;
    }

    return out;
}

QString bracketCheckMessage(const QString &cleanedCode,
                            QChar left,
                            QChar right,
                            const QString &name) {
    int count = 0;

    for (QChar ch : cleanedCode) {
        if (ch == left) {
            ++count;
        } else if (ch == right) {
            --count;
            if (count < 0) {
                return QString("- %1 不匹配：出现了多余的 `%2`。\n")
                    .arg(name)
                    .arg(right);
            }
        }
    }

    if (count > 0) {
        return QString("- %1 不匹配：缺少 `%2`。\n")
            .arg(name)
            .arg(right);
    }

    return "";
}

int findMatchingBraceInCleanCode(const QString &code, int openPos) {
    int depth = 0;

    for (int i = openPos; i < code.size(); ++i) {
        if (code[i] == '{') {
            ++depth;
        } else if (code[i] == '}') {
            --depth;
            if (depth == 0) return i;
        }
    }

    return -1;
}

QChar nextNonSpaceChar(const QString &code, int pos) {
    for (int i = pos; i < code.size(); ++i) {
        if (!code[i].isSpace()) return code[i];
    }
    return QChar();
}

QString classSemicolonCheckMessage(const QString &cleanedCode) {
    QRegularExpression classRe(
        R"(\b(class|struct)\s+[A-Za-z_]\w*(\s*:\s*[^{};]+)?\s*\{)"
        );

    QRegularExpressionMatchIterator it = classRe.globalMatch(cleanedCode);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        int openBrace = cleanedCode.indexOf('{', match.capturedStart());
        if (openBrace < 0) continue;

        int closeBrace = findMatchingBraceInCleanCode(cleanedCode, openBrace);
        if (closeBrace < 0) continue;

        QChar after = nextNonSpaceChar(cleanedCode, closeBrace + 1);
        if (after != ';') {
            QString classHeader = match.captured(0).trimmed();
            classHeader.replace('\n', ' ');

            return QString("- 类定义结尾可能缺少分号 `;`。\n"
                           "  出问题的位置大概在：%1 ... }\n"
                           "  正确写法应该是：class A { ... };\n")
                .arg(classHeader);
        }
    }

    return "";
}

QString missingIncludeHints(const QString &code) {
    QString hints;

    bool hasAlgorithmInclude = code.contains(QRegularExpression(
        R"(^\s*#\s*include\s*<algorithm>)", QRegularExpression::MultilineOption));
    bool hasArrayInclude = code.contains(QRegularExpression(
        R"(^\s*#\s*include\s*<array>)", QRegularExpression::MultilineOption));
    bool hasDequeInclude = code.contains(QRegularExpression(
        R"(^\s*#\s*include\s*<deque>)", QRegularExpression::MultilineOption));
    bool hasIostreamInclude = code.contains(QRegularExpression(
        R"(^\s*#\s*include\s*<iostream>)", QRegularExpression::MultilineOption));
    bool hasListInclude = code.contains(QRegularExpression(
        R"(^\s*#\s*include\s*<list>)", QRegularExpression::MultilineOption));
    bool hasStringInclude = code.contains(QRegularExpression(
        R"(^\s*#\s*include\s*<string>)", QRegularExpression::MultilineOption));
    bool hasVectorInclude = code.contains(QRegularExpression(
        R"(^\s*#\s*include\s*<vector>)", QRegularExpression::MultilineOption));
    bool hasMapInclude = code.contains(QRegularExpression(
        R"(^\s*#\s*include\s*<map>)", QRegularExpression::MultilineOption));
    bool hasMemoryInclude = code.contains(QRegularExpression(
        R"(^\s*#\s*include\s*<memory>)", QRegularExpression::MultilineOption));
    bool hasSetInclude = code.contains(QRegularExpression(
        R"(^\s*#\s*include\s*<set>)", QRegularExpression::MultilineOption));
    bool hasUnorderedMapInclude = code.contains(QRegularExpression(
        R"(^\s*#\s*include\s*<unordered_map>)", QRegularExpression::MultilineOption));
    bool hasUnorderedSetInclude = code.contains(QRegularExpression(
        R"(^\s*#\s*include\s*<unordered_set>)", QRegularExpression::MultilineOption));
    bool hasBitsInclude = code.contains(QRegularExpression(
        R"(^\s*#\s*include\s*<bits/stdc\+\+\.h>)", QRegularExpression::MultilineOption));

    if (!hasBitsInclude && !hasAlgorithmInclude &&
        code.contains(QRegularExpression(R"(\bstd::(sort|find|max|min|reverse)\b|\b(sort|reverse)\s*\()"))) {
        hints += "- 检测到 algorithm 常用函数，建议添加：#include <algorithm>\n";
    }

    if (!hasBitsInclude && !hasArrayInclude &&
        code.contains(QRegularExpression(R"(\bstd::array\b|\barray\s*<)"))) {
        hints += "- 检测到 array / std::array，建议添加：#include <array>\n";
    }

    if (!hasBitsInclude && !hasDequeInclude &&
        code.contains(QRegularExpression(R"(\bstd::deque\b|\bdeque\s*<)"))) {
        hints += "- 检测到 deque / std::deque，建议添加：#include <deque>\n";
    }

    if (!hasBitsInclude && !hasIostreamInclude &&
        code.contains(QRegularExpression(R"(\b(std::)?(cout|cin|cerr|endl)\b)"))) {
        hints += "- 检测到 cout / cin / endl，建议添加：#include <iostream>\n";
    }

    if (!hasBitsInclude && !hasListInclude &&
        code.contains(QRegularExpression(R"(\bstd::list\b|\blist\s*<)"))) {
        hints += "- 检测到 list / std::list，建议添加：#include <list>\n";
    }

    if (!hasBitsInclude && !hasStringInclude &&
        code.contains(QRegularExpression(R"(\b(std::)?string\b)"))) {
        hints += "- 检测到 string / std::string，建议添加：#include <string>\n";
    }

    if (!hasBitsInclude && !hasVectorInclude &&
        code.contains(QRegularExpression(R"(\bstd::vector\b|\bvector\s*<)"))) {
        hints += "- 检测到 vector / std::vector，建议添加：#include <vector>\n";
    }

    if (!hasBitsInclude && !hasMapInclude &&
        code.contains(QRegularExpression(R"(\bstd::map\b|\bmap\s*<)"))) {
        hints += "- 检测到 map / std::map，建议添加：#include <map>\n";
    }

    if (!hasBitsInclude && !hasMemoryInclude &&
        code.contains(QRegularExpression(R"(\bstd::(unique_ptr|shared_ptr|weak_ptr|make_unique|make_shared)\b|\b(unique_ptr|shared_ptr|weak_ptr)\s*<)"))) {
        hints += "- 检测到智能指针，建议添加：#include <memory>\n";
    }

    if (!hasBitsInclude && !hasSetInclude &&
        code.contains(QRegularExpression(R"(\bstd::set\b|\bset\s*<)"))) {
        hints += "- 检测到 set / std::set，建议添加：#include <set>\n";
    }

    if (!hasBitsInclude && !hasUnorderedMapInclude &&
        code.contains(QRegularExpression(R"(\bstd::unordered_map\b|\bunordered_map\s*<)"))) {
        hints += "- 检测到 unordered_map，建议添加：#include <unordered_map>\n";
    }

    if (!hasBitsInclude && !hasUnorderedSetInclude &&
        code.contains(QRegularExpression(R"(\bstd::unordered_set\b|\bunordered_set\s*<)"))) {
        hints += "- 检测到 unordered_set，建议添加：#include <unordered_set>\n";
    }

    return hints;
}

QString commonSyntaxErrors(const QString &code) {
    QString errors;
    QString cleaned = removeCommentsAndStringLiteralsForCheck(code);

    if (code.contains("；")) {
        errors += "- 检测到中文分号 `；`，C++ 里应该使用英文分号 `;`。\n";
    }

    if (code.contains("：")) {
        errors += "- 检测到中文冒号 `：`，C++ 里应该使用英文冒号 `:`。\n";
    }

    errors += bracketCheckMessage(cleaned, '{', '}', "大括号");
    errors += bracketCheckMessage(cleaned, '(', ')', "小括号");
    errors += bracketCheckMessage(cleaned, '[', ']', "中括号");

    if (cleaned.contains(QRegularExpression(R"(\b(class|struct)\s*\{)"))) {
        errors += "- 检测到 class / struct 后面没有类名，例如 `class { ... };` 是不合法的。\n";
    }

    errors += classSemicolonCheckMessage(cleaned);
    return errors;
}

QString syntaxErrorMessage(const QString &syntaxErrors, const QString &includeHints) {
    QString msg;
    msg += "【常见语法错误检查】\n";
    msg += syntaxErrors;

    if (!includeHints.isEmpty()) {
        msg += "\n【可能缺少的头文件】\n";
        msg += includeHints;
    }

    msg += "\n程序暂不继续分析。请先修改上述问题。";
    return msg;
}

QString unsupportedSyntaxWarnings(const QString &code) {
    QString warnings;

    QRegularExpression templateRe(
        R"(\btemplate\s*<[^>]*>\s*(class|struct)\b)",
        QRegularExpression::DotMatchesEverythingOption
        );
    if (templateRe.match(code).hasMatch()) {
        warnings += "- 检测到 template 模板类：当前版本暂不支持模板类解析。建议先写成已经确定类型的普通类，例如把 `Box<T>` 改成 `BoxInt`。\n";
    }

    QRegularExpression virtualInheritanceRe(
        R"(\b(class|struct)\s+[A-Za-z_]\w*\s*:\s*[^{};]*\bvirtual\b)"
        );
    if (virtualInheritanceRe.match(code).hasMatch()) {
        warnings += "- 检测到虚继承：为保证结果正确，暂不支持分析虚继承布局。建议改成普通非虚继承后再观察对象布局。\n";
    }

    QRegularExpression namespaceRe(
        R"(\bnamespace\s+[A-Za-z_]\w*\s*\{)"
        );
    if (namespaceRe.match(code).hasMatch()) {
        warnings += "- 检测到 namespace：当前版本对命名空间内类的完整限定名支持有限。建议先把要分析的类移到全局作用域。\n";
    }

    QRegularExpression unionRe(
        R"(\bunion\s+[A-Za-z_]\w*\s*\{)"
        );
    if (unionRe.match(code).hasMatch()) {
        warnings += "- 检测到 union：当前版本暂不支持 union 内存布局分析。建议改成普通 class / struct 后再分析。\n";
    }

    QRegularExpression nestedClassRe(
        R"(\b(class|struct)\s+[A-Za-z_]\w*[^{}]*\{[^{}]*\b(class|struct)\s+[A-Za-z_]\w*)",
        QRegularExpression::DotMatchesEverythingOption
        );
    if (nestedClassRe.match(code).hasMatch()) {
        warnings += "- 检测到嵌套类：当前版本暂不支持嵌套类解析。建议把内部类移到外部，作为单独的普通类分析。\n";
    }

    QRegularExpression bitFieldRe(
        R"(\b[A-Za-z_:][\w:\s<>,*&]*\s+[A-Za-z_]\w*\s*:\s*\d+\s*[;,])"
        );
    if (bitFieldRe.match(code).hasMatch()) {
        warnings += "- 检测到 bit-field 位域：位域成员不能取地址，当前探针无法用字段地址测量精确 offset。建议改成普通整型成员后再分析。\n";
    }

    return warnings;
}

QString supportMessageForEmptyParse(const QString &warnings) {
    QString msg;

    if (!warnings.isEmpty()) {
        msg += "【语法支持提示】\n";
        msg += warnings;
        msg += "\n";
    }

    msg += "未解析到可分析的普通类。\n\n";
    msg += "当前版本主要支持普通 class / struct，例如：\n\n";
    msg += "class A {\n";
    msg += "public:\n";
    msg += "    int x;\n";
    msg += "};\n\n";
    msg += "暂不支持或支持有限的语法包括：\n";
    msg += "- template 模板类\n";
    msg += "- 虚继承\n";
    msg += "- 嵌套类\n";
    msg += "- union\n";
    msg += "- bit-field 位域\n";
    msg += "- namespace 中的复杂类名\n";

    return msg;
}

QString extractCompilerOutput(const QString &engineError) {
    int marker = engineError.indexOf("【clang++ 输出】");
    if (marker < 0) return engineError.trimmed();

    return engineError.mid(marker + QString("【clang++ 输出】").size()).trimmed();
}

QString compilerOutputDiagnosis(const QString &engineError,
                                const QString &includeHints) {
    QString clangOutput = extractCompilerOutput(engineError);
    QString lower = clangOutput.toLower();
    QString reasons;
    QString fixes;

    auto addPair = [&](const QString &reason, const QString &fix) {
        if (!reasons.contains(reason)) reasons += "- " + reason + "\n";
        if (!fixes.contains(fix)) fixes += "- " + fix + "\n";
    };

    if (!includeHints.trimmed().isEmpty()) {
        addPair("代码里使用了需要头文件的标准库类型或函数，但当前代码可能没有包含对应头文件。",
                "按“可能缺少的头文件”列表添加 #include，然后重新分析。");
    }

    if (lower.contains("no type named") ||
        lower.contains("unknown type name") ||
        lower.contains("use of undeclared identifier") ||
        lower.contains("undeclared identifier")) {
        addPair("编译器找不到某个类型、变量或函数名。",
                "检查名字是否拼写错误；如果是标准库类型，请补上对应 #include 和 std:: 前缀。");
    }

    if (lower.contains("no template named") ||
        lower.contains("implicit instantiation of undefined template") ||
        lower.contains("implicit instantiation of undefined template")) {
        addPair("标准库模板类型没有完整定义，常见原因是缺少头文件。",
                "例如 std::vector 需要 #include <vector>，std::string 需要 #include <string>。");
    }

    if (lower.contains("expected ';'") ||
        lower.contains("expected member name") ||
        lower.contains("expected expression") ||
        lower.contains("expected unqualified-id")) {
        addPair("用户代码里可能有 C++ 语法错误。",
                "优先检查类定义结尾是否有分号、括号是否配对、成员声明是否以分号结束。");
    }

    if (lower.contains("cannot take the address of bit-field") ||
        lower.contains("address of bit-field")) {
        addPair("代码中包含 bit-field 位域成员；位域不能取地址，因此当前探针无法测量字段地址。",
                "把位域临时改成普通整型成员，例如把 `unsigned x : 3;` 改成 `unsigned x;`。");
    }

    if (lower.contains("incomplete type") ||
        lower.contains("forward declaration")) {
        addPair("某个字段类型只有前置声明，没有完整定义，sizeof 无法得到真实大小。",
                "在被分析类之前提供该类型的完整 class / struct 定义，或把字段改成指针类型。");
    }

    if (lower.contains("is a private member") ||
        lower.contains("is a protected member") ||
        lower.contains("private base class") ||
        lower.contains("protected base class") ||
        lower.contains("inaccessible")) {
        addPair("探针访问成员或父类子对象时遇到访问控制限制。",
                "可以先把要分析的字段或继承关系改成 public；若代码本身很复杂，建议提取一个最小可分析样例。");
    }

    if (lower.contains("ambiguous") &&
        (lower.contains("base") || lower.contains("member"))) {
        addPair("多继承或菱形继承中出现了二义性访问。",
                "为同名成员标注清楚继承路径，或简化为一个最小多继承样例后再分析。");
    }

    if (lower.contains("invalid application of 'sizeof'") ||
        lower.contains("invalid application of sizeof")) {
        addPair("某个字段或类型不能被 sizeof 正常测量。",
                "检查字段是否为不完整类型、函数类型、位域，或当前版本暂不支持的复杂声明。");
    }

    if ((lower.contains("clang++") && lower.contains("no such file")) ||
        lower.contains("error: no input files")) {
        addPair("本机编译环境可能不可用或临时探针文件生成失败。",
                "确认已经安装 Xcode Command Line Tools，并重新运行程序。");
    }

    if (reasons.isEmpty()) {
        reasons += "- 探针代码没有通过编译。可能是输入代码本身编译不过，或使用了当前版本暂不支持的复杂 C++ 语法。\n";
    }

    if (fixes.isEmpty()) {
        fixes += "- 先把代码简化成只包含 class / struct 定义、字段和简单成员函数的最小样例，再逐步加回复杂语法。\n";
    }

    QString msg;
    msg += "【错误原因】\n";
    msg += reasons;
    msg += "\n【修改建议】\n";
    msg += fixes;

    if (!clangOutput.isEmpty()) {
        msg += "\n【原始 clang++ 输出】\n";
        msg += clangOutput;
    }

    return msg;
}

QString analysisFailureMessage(const QString &warnings,
                               const QString &includeHints,
                               const QString &engineError) {
    QString msg;

    if (!warnings.isEmpty()) {
        msg += "【语法支持提示】\n";
        msg += warnings;
        msg += "\n";
    }

    if (!includeHints.isEmpty()) {
        msg += "【可能缺少的头文件】\n";
        msg += includeHints;
        msg += "\n";
    }

    msg += "【探针编译或运行失败】\n";

    if (engineError.trimmed().isEmpty()) {
        msg += "【错误原因】\n";
        msg += "- 没有收到详细错误信息，可能是探针编译或运行被系统提前中断。\n\n";
        msg += "【修改建议】\n";
        msg += "- 检查是否缺少 include，或是否使用了模板、虚继承、嵌套类等暂不支持语法。\n";
    } else {
        msg += compilerOutputDiagnosis(engineError, includeHints);
    }

    return msg;
}

void setExampleCode(QPlainTextEdit *codeEdit) {
    codeEdit->setPlainText(defaultExampleCode());
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    applyModernStyle();
    setWindowTitle("C++ Memory Visualizer");
    resize(1280, 720);
    setMinimumSize(1120, 640);

    auto layoutsStore = std::make_shared<QVector<ClassLayout>>();
    QNetworkAccessManager *aiManager = new QNetworkAccessManager(this);

    QWidget *central = new QWidget(this);
    QVBoxLayout *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(16, 14, 16, 14);
    rootLayout->setSpacing(10);

    QHBoxLayout *topBar = new QHBoxLayout();

    QVBoxLayout *titleBox = new QVBoxLayout();
    titleBox->setSpacing(1);
    QLabel *title = makeTitle("C++ Memory Visualizer", central);
    QLabel *subTitle = makeSubTitle("Class parser · object layout · virtual functions", central);
    titleBox->addWidget(title);
    titleBox->addWidget(subTitle);

    QPushButton *exampleButton = new QPushButton("载入示例", central);
    exampleButton->setObjectName("SecondaryButton");

    QPushButton *clearButton = new QPushButton("清空", central);
    clearButton->setObjectName("SecondaryButton");

    QPushButton *aiButton = new QPushButton("AI 助教", central);
    aiButton->setObjectName("SecondaryButton");

    QPushButton *exportButton = new QPushButton("导出报告", central);
    exportButton->setObjectName("SecondaryButton");

    QPushButton *analyzeButton = new QPushButton("分析代码", central);

    topBar->addLayout(titleBox);
    topBar->addStretch();
    topBar->addWidget(exampleButton);
    topBar->addWidget(clearButton);
    topBar->addWidget(aiButton);
    topBar->addWidget(exportButton);
    topBar->addWidget(analyzeButton);

    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, central);

    QFrame *codePanel = makePanel(central);
    codePanel->setMinimumWidth(305);
    addPanelGlow(codePanel, QColor(14, 165, 233, 38));
    QVBoxLayout *codeLayout = new QVBoxLayout(codePanel);
    codeLayout->setContentsMargins(10, 10, 10, 10);
    codeLayout->setSpacing(8);
    QLabel *codeTitle = makeSubTitle("Input C++ Code", codePanel);

    QPlainTextEdit *codeEdit = new QPlainTextEdit(codePanel);
    codeEdit->setPlaceholderText("在这里输入 C++ 类定义...");
    codeEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    QFont codeFont("Menlo");
    codeFont.setPointSize(8);
    codeEdit->setFont(codeFont);
    codeEdit->setTabStopDistance(4 * codeEdit->fontMetrics().horizontalAdvance(' '));

    codeLayout->addWidget(codeTitle);
    codeLayout->addWidget(codeEdit);

    QFrame *classPanel = makePanel(central);
    classPanel->setMinimumWidth(155);
    addPanelGlow(classPanel, QColor(14, 165, 233, 32));
    QVBoxLayout *classLayout = new QVBoxLayout(classPanel);
    classLayout->setContentsMargins(10, 10, 10, 10);
    classLayout->setSpacing(8);
    QLabel *classTitle = makeSubTitle("Classes", classPanel);
    QListWidget *classList = new QListWidget(classPanel);
    classList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    classList->setMinimumWidth(130);

    QTextEdit *summaryEdit = new QTextEdit(classPanel);
    summaryEdit->setReadOnly(true);
    summaryEdit->setMaximumHeight(140);
    summaryEdit->setPlaceholderText("类摘要信息会显示在这里");

    classLayout->addWidget(classTitle);
    classLayout->addWidget(classList, 3);
    classLayout->addWidget(makeSubTitle("Summary", classPanel));
    classLayout->addWidget(summaryEdit, 1);

    QFrame *resultPanel = makePanel(central);
    resultPanel->setMinimumWidth(520);
    addPanelGlow(resultPanel, QColor(168, 85, 247, 36));
    QVBoxLayout *resultLayout = new QVBoxLayout(resultPanel);
    resultLayout->setContentsMargins(8, 8, 8, 8);
    resultLayout->setSpacing(2);
    QLabel *resultTitle = makeSubTitle("Analysis Result", resultPanel);

    QGraphicsScene *memoryScene = new QGraphicsScene(resultPanel);
    QGraphicsView *memoryView = new QGraphicsView(memoryScene, resultPanel);
    memoryView->setRenderHint(QPainter::Antialiasing);
    memoryView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    memoryView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    memoryView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    memoryView->setResizeAnchor(QGraphicsView::AnchorViewCenter);

    QGraphicsScene *vtableScene = new QGraphicsScene(resultPanel);
    QGraphicsView *vtableView = new QGraphicsView(vtableScene, resultPanel);
    vtableView->setRenderHint(QPainter::Antialiasing);

    QTextEdit *optimizationEdit = new QTextEdit(resultPanel);
    optimizationEdit->setReadOnly(true);

    QTableWidget *fieldTable = new QTableWidget(resultPanel);
    QTableWidget *baseTable = new QTableWidget(resultPanel);
    QTableWidget *functionTable = new QTableWidget(resultPanel);
    configureDataTable(fieldTable);
    configureDataTable(baseTable);
    configureDataTable(functionTable);
    QFrame *fieldTableFrame = makeTableFrame(fieldTable, resultPanel);
    QFrame *baseTableFrame = makeTableFrame(baseTable, resultPanel);
    QFrame *functionTableFrame = makeTableFrame(functionTable, resultPanel);

    QTabWidget *tabs = new QTabWidget(resultPanel);
    tabs->addTab(memoryView, "可视化");
    tabs->addTab(vtableView, "虚函数表");
    tabs->addTab(optimizationEdit, "优化建议");
    tabs->addTab(baseTableFrame, "父类子对象");
    tabs->addTab(fieldTableFrame, "成员变量");
    tabs->addTab(functionTableFrame, "成员函数");

    resultTitle->setMaximumHeight(18);
    resultLayout->addWidget(resultTitle);
    resultLayout->addWidget(tabs);

    QHBoxLayout *pageLayout = new QHBoxLayout();
    QPushButton *prevPageButton = new QPushButton("上一页", resultPanel);
    QPushButton *nextPageButton = new QPushButton("下一页", resultPanel);
    QLabel *pageLabel = makeSubTitle("字段页：1 / 1", resultPanel);
    QPushButton *prevVTablePageButton = new QPushButton("上一虚表", resultPanel);
    QPushButton *nextVTablePageButton = new QPushButton("下一虚表", resultPanel);
    QLabel *vtablePageLabel = makeSubTitle("虚表页：1 / 1", resultPanel);
    prevPageButton->setObjectName("SecondaryButton");
    nextPageButton->setObjectName("SecondaryButton");
    prevVTablePageButton->setObjectName("SecondaryButton");
    nextVTablePageButton->setObjectName("SecondaryButton");
    prevPageButton->setFixedSize(52, 24);
    nextPageButton->setFixedSize(52, 24);
    prevVTablePageButton->setFixedSize(64, 24);
    nextVTablePageButton->setFixedSize(64, 24);
    const QString pagerButtonStyle =
        "QPushButton { font-size: 11px; padding: 0px; border-radius: 6px; }";
    prevPageButton->setStyleSheet(pagerButtonStyle);
    nextPageButton->setStyleSheet(pagerButtonStyle);
    prevVTablePageButton->setStyleSheet(pagerButtonStyle);
    nextVTablePageButton->setStyleSheet(pagerButtonStyle);
    pageLayout->addStretch();
    pageLayout->addWidget(prevPageButton);
    pageLayout->addWidget(pageLabel);
    pageLayout->addWidget(nextPageButton);
    pageLayout->addSpacing(18);
    pageLayout->addWidget(prevVTablePageButton);
    pageLayout->addWidget(vtablePageLabel);
    pageLayout->addWidget(nextVTablePageButton);
    resultLayout->addLayout(pageLayout);

    mainSplitter->addWidget(codePanel);
    mainSplitter->addWidget(classPanel);
    mainSplitter->addWidget(resultPanel);
    mainSplitter->setStretchFactor(0, 3);
    mainSplitter->setStretchFactor(1, 1);
    mainSplitter->setStretchFactor(2, 5);
    mainSplitter->setSizes({360, 220, 700});

    QLabel *statusLabel = makeSubTitle("状态：等待输入代码", central);

    rootLayout->addLayout(topBar);
    rootLayout->addWidget(mainSplitter, 1);
    rootLayout->addWidget(statusLabel);

    setCentralWidget(central);

    setExampleCode(codeEdit);

    auto currentFieldPage = std::make_shared<int>(0);
    auto currentVTablePage = std::make_shared<int>(0);

    auto renderSelectedClass = [=](int row) {
        if (row < 0 || row >= layoutsStore->size()) return;

        const ClassLayout &layoutInfo = layoutsStore->at(row);
        int pages = fieldPageCount(layoutInfo);
        int vtablePages = vtablePageCount(layoutInfo);
        *currentFieldPage = std::max(0, std::min(*currentFieldPage, pages - 1));
        *currentVTablePage = std::max(0, std::min(*currentVTablePage, vtablePages - 1));
        pageLabel->setText(QString("字段页：%1 / %2").arg(*currentFieldPage + 1).arg(pages));
        vtablePageLabel->setText(QString("虚表页：%1 / %2").arg(*currentVTablePage + 1).arg(vtablePages));
        prevPageButton->setEnabled(*currentFieldPage > 0);
        nextPageButton->setEnabled(*currentFieldPage + 1 < pages);
        prevVTablePageButton->setEnabled(layoutInfo.vtable.size() > 3 && *currentVTablePage > 0);
        nextVTablePageButton->setEnabled(layoutInfo.vtable.size() > 3 && *currentVTablePage + 1 < vtablePages);

        summaryEdit->setPlainText(makeClassSummary(layoutInfo));
        fillFieldTable(fieldTable, layoutInfo, [=](int fieldRow) {
            int classRow = classList->currentRow();
            if (classRow < 0 || classRow >= layoutsStore->size()) return;

            const ClassLayout &selectedLayout = layoutsStore->at(classRow);
            if (fieldRow < 0 || fieldRow >= static_cast<int>(selectedLayout.fields.size())) return;

            showTextDialog(this, "字段解释", explainField(selectedLayout, selectedLayout.fields[fieldRow]));
        });
        fillBaseTable(baseTable, layoutInfo);
        fillFunctionTable(functionTable, layoutInfo);
        optimizationEdit->setPlainText(paddingOptimizationText(layoutInfo));
        drawVTable(vtableScene, layoutInfo);

        QTimer::singleShot(0, this, [=]() {
            if (row < 0 || row >= layoutsStore->size()) return;
            const ClassLayout &delayedLayout = layoutsStore->at(row);
            drawMemoryLayout(memoryScene,
                             delayedLayout,
                             memoryView->viewport()->size(),
                             *currentFieldPage,
                             *currentVTablePage);
            memoryView->fitInView(memoryScene->sceneRect(), Qt::KeepAspectRatio);
        });
    };

    auto showClass = [=](int row) {
        *currentFieldPage = 0;
        *currentVTablePage = 0;
        renderSelectedClass(row);
    };

    connect(classList, &QListWidget::currentRowChanged, this, showClass);
    connect(mainSplitter, &QSplitter::splitterMoved, this, [=]() {
        renderSelectedClass(classList->currentRow());
    });
    connect(prevPageButton, &QPushButton::clicked, this, [=]() {
        if (*currentFieldPage > 0) {
            --(*currentFieldPage);
            renderSelectedClass(classList->currentRow());
        }
    });
    connect(nextPageButton, &QPushButton::clicked, this, [=]() {
        int row = classList->currentRow();
        if (row < 0 || row >= layoutsStore->size()) return;
        int pages = fieldPageCount(layoutsStore->at(row));
        if (*currentFieldPage + 1 < pages) {
            ++(*currentFieldPage);
            renderSelectedClass(row);
        }
    });
    connect(prevVTablePageButton, &QPushButton::clicked, this, [=]() {
        if (*currentVTablePage > 0) {
            --(*currentVTablePage);
            renderSelectedClass(classList->currentRow());
        }
    });
    connect(nextVTablePageButton, &QPushButton::clicked, this, [=]() {
        int row = classList->currentRow();
        if (row < 0 || row >= layoutsStore->size()) return;
        int pages = vtablePageCount(layoutsStore->at(row));
        if (*currentVTablePage + 1 < pages) {
            ++(*currentVTablePage);
            renderSelectedClass(row);
        }
    });

    connect(exampleButton, &QPushButton::clicked, this, [=]() {
        QMenu menu(exampleButton);
        const QVector<ExampleSnippet> examples = exampleSnippets();
        for (const ExampleSnippet &example : examples) {
            QAction *action = menu.addAction(example.title);
            action->setToolTip(example.goal);
            QObject::connect(action, &QAction::triggered, this, [=]() {
                codeEdit->setPlainText(example.code);
                classList->clear();
                summaryEdit->clear();
                optimizationEdit->clear();
                fieldTable->clear();
                baseTable->clear();
                functionTable->clear();
                memoryScene->clear();
                vtableScene->clear();
                layoutsStore->clear();
                statusLabel->setText("状态：已载入示例代码 - " + example.title);
            });
        }
        menu.exec(exampleButton->mapToGlobal(QPoint(0, exampleButton->height())));
    });

    connect(clearButton, &QPushButton::clicked, this, [=]() {
        codeEdit->clear();
        classList->clear();
        summaryEdit->clear();
        optimizationEdit->clear();
        fieldTable->clear();
        baseTable->clear();
        functionTable->clear();
        memoryScene->clear();
        vtableScene->clear();
        layoutsStore->clear();
        statusLabel->setText("状态：已清空");
    });

    connect(exportButton, &QPushButton::clicked, this, [=]() {
        if (layoutsStore->isEmpty()) {
            QMessageBox::information(this, "导出报告", "请先点击“分析代码”，得到布局结果后再导出报告。");
            return;
        }

        QString defaultName = QString("cppmem_report_%1.md")
            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
        QString path = QFileDialog::getSaveFileName(
            this,
            "导出 Markdown 报告",
            defaultName,
            "Markdown Files (*.md);;Text Files (*.txt)");
        if (path.isEmpty()) {
            return;
        }

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "导出报告", "无法写入报告文件：" + path);
            return;
        }

        QTextStream out(&file);
        out << makeReport(codeEdit->toPlainText(), *layoutsStore);
        file.close();

        statusLabel->setText("状态：Markdown 报告已导出：" + path);
        showExportSuccessDialog(this);
    });

    connect(aiButton, &QPushButton::clicked, this, [=]() {
        if (layoutsStore->isEmpty()) {
            QMessageBox::information(this, "AI 助教", "请先点击“分析代码”，得到布局结果后再让 AI 助教讲解。");
            return;
        }

        int selectedRow = classList->currentRow();
        QString fallback = localTutorText(codeEdit->toPlainText(), *layoutsStore, selectedRow);

        QDialog *dialog = new QDialog(this);
        dialog->setWindowTitle("AI 助教讲解");
        dialog->resize(720, 560);

        QVBoxLayout *dialogLayout = new QVBoxLayout(dialog);
        QLabel *hint = makeSubTitle("根据当前代码和解析结果生成讲解", dialog);
        QHBoxLayout *providerLayout = new QHBoxLayout();
        QLabel *providerLabel = makeSubTitle("AI 服务", dialog);
        QComboBox *providerBox = new QComboBox(dialog);
        providerBox->addItems({"OpenAI", "DeepSeek", "通义千问", "Kimi", "Ollama 本地"});
        QLineEdit *modelEdit = new QLineEdit(dialog);
        modelEdit->setPlaceholderText("模型名");
        modelEdit->setText(defaultModelForProvider(providerBox->currentText()));
        providerLayout->addWidget(providerLabel);
        providerLayout->addWidget(providerBox, 1);
        providerLayout->addWidget(makeSubTitle("模型", dialog));
        providerLayout->addWidget(modelEdit, 2);

        QLineEdit *apiKeyEdit = new QLineEdit(dialog);
        apiKeyEdit->setPlaceholderText("可选：在这里粘贴所选服务的 API Key，然后点“联网讲解”");
        apiKeyEdit->setEchoMode(QLineEdit::Password);
        apiKeyEdit->setText(QString::fromUtf8(qgetenv(envKeyForProvider(providerBox->currentText()).toUtf8().constData())));
        QTextEdit *output = new QTextEdit(dialog);
        output->setReadOnly(true);

        QPushButton *requestButton = new QPushButton("联网讲解", dialog);
        requestButton->setObjectName("SecondaryButton");
        QPushButton *closeButton = new QPushButton("关闭", dialog);
        closeButton->setObjectName("SecondaryButton");
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        buttonLayout->addStretch();
        buttonLayout->addWidget(requestButton);
        buttonLayout->addWidget(closeButton);

        dialogLayout->addWidget(hint);
        dialogLayout->addLayout(providerLayout);
        dialogLayout->addWidget(apiKeyEdit);
        dialogLayout->addWidget(output, 1);
        dialogLayout->addLayout(buttonLayout);
        connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
        connect(providerBox, &QComboBox::currentTextChanged, this, [=](const QString &provider) {
            modelEdit->setText(defaultModelForProvider(provider));
            QString envKey = envKeyForProvider(provider);
            apiKeyEdit->setText(envKey.isEmpty()
                                    ? ""
                                    : QString::fromUtf8(qgetenv(envKey.toUtf8().constData())));
            apiKeyEdit->setEnabled(provider != "Ollama 本地");
            apiKeyEdit->setPlaceholderText(provider == "Ollama 本地"
                                           ? "Ollama 本地模型不需要 API Key，请先启动 ollama serve"
                                           : "可选：在这里粘贴所选服务的 API Key，然后点“联网讲解”");
        });

        dialog->show();
        output->setPlainText(fallback);

        auto requestAiExplanation = [=]() {
            AiRequestConfig config = buildAiRequestConfig(providerBox->currentText(),
                                                          modelEdit->text(),
                                                          apiKeyEdit->text(),
                                                          buildAiPrompt(codeEdit->toPlainText(), *layoutsStore, selectedRow));
            if (config.needsApiKey && config.apiKey.isEmpty()) {
                output->setPlainText(fallback + "\n\n要调用所选大模型服务，请先在上方输入对应 API Key。");
                return;
            }
            if (!config.url.isValid()) {
                output->setPlainText(fallback + "\n\n所选 AI 服务暂未配置可用地址。");
                return;
            }

            output->setPlainText(QString("正在请求 %1 / %2，请稍等...")
                                     .arg(config.provider, config.model));

            QNetworkRequest request(config.url);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            if (!config.apiKey.isEmpty()) {
                request.setRawHeader("Authorization", "Bearer " + config.apiKey);
            }

            QPointer<QTextEdit> outputPtr(output);
            QNetworkReply *reply = aiManager->post(request, QJsonDocument(config.body).toJson(QJsonDocument::Compact));
            connect(reply, &QNetworkReply::finished, this, [reply, outputPtr, fallback, provider = config.provider]() {
                QByteArray body = reply->readAll();
                if (!outputPtr) {
                    reply->deleteLater();
                    return;
                }

                if (reply->error() != QNetworkReply::NoError) {
                    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                    QString detail = reply->errorString();
                    if (statusCode == 429) {
                        detail = QString("请求已经发到 %1，但账号当前没有可用额度、达到限速，或项目计费未启用。请检查该服务的额度/计费设置后再试。")
                                     .arg(provider);
                    } else if (provider == "Ollama 本地") {
                        detail += "。如果使用 Ollama，请确认本机已经安装模型并启动 ollama serve。";
                    }
                    outputPtr->setPlainText(QString("%1\n\n联网 AI 请求失败：%2")
                                                .arg(fallback, detail));
                    reply->deleteLater();
                    return;
                }

                QJsonParseError parseError;
                QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
                if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
                    outputPtr->setPlainText(QString("%1\n\nAI 返回内容无法解析，已显示本地讲解。")
                                                .arg(fallback));
                    reply->deleteLater();
                    return;
                }

                QString aiText = extractOpenAIText(doc.object()).trimmed();
                if (aiText.isEmpty()) {
                    outputPtr->setPlainText(QString("%1\n\nAI 返回为空，已显示本地讲解。")
                                                .arg(fallback));
                } else {
                    outputPtr->setPlainText(aiText);
                }
                reply->deleteLater();
            });
        };

        connect(requestButton, &QPushButton::clicked, this, requestAiExplanation);
        if (!apiKeyEdit->text().trimmed().isEmpty()) {
            requestAiExplanation();
        }
    });

    connect(analyzeButton, &QPushButton::clicked, this, [=]() {
        QString code = codeEdit->toPlainText();

        if (code.trimmed().isEmpty()) {
            statusLabel->setText("状态：请输入 C++ 类定义");
            summaryEdit->setPlainText("请输入 C++ 类定义。");
            optimizationEdit->clear();
            tabs->setCurrentIndex(0);
            return;
        }

        QString syntaxErrors = commonSyntaxErrors(code);
        QString includeHints = missingIncludeHints(code);

        if (!syntaxErrors.isEmpty()) {
            statusLabel->setText("状态：检测到常见 C++ 语法错误");
            summaryEdit->setPlainText(syntaxErrorMessage(syntaxErrors, includeHints));
            tabs->setCurrentIndex(0);

            classList->clear();
            optimizationEdit->clear();
            fieldTable->clear();
            baseTable->clear();
            functionTable->clear();
            memoryScene->clear();
            vtableScene->clear();
            layoutsStore->clear();
            return;
        }

        QString warnings = unsupportedSyntaxWarnings(code);
        if (!warnings.isEmpty()) {
            statusLabel->setText("状态：检测到当前暂不支持的 C++ 语法");
            summaryEdit->setPlainText(supportMessageForEmptyParse(warnings));
            tabs->setCurrentIndex(0);

            classList->clear();
            optimizationEdit->clear();
            fieldTable->clear();
            baseTable->clear();
            functionTable->clear();
            memoryScene->clear();
            vtableScene->clear();
            layoutsStore->clear();
            return;
        }

        ClassParser parser;
        QVector<RawClassInfo> classes = parser.parse(code);

        if (classes.isEmpty()) {
            statusLabel->setText("状态：未解析到任何类，请检查代码格式");
            summaryEdit->setPlainText(supportMessageForEmptyParse(""));
            tabs->setCurrentIndex(0);

            classList->clear();
            optimizationEdit->clear();
            fieldTable->clear();
            baseTable->clear();
            functionTable->clear();
            memoryScene->clear();
            vtableScene->clear();
            layoutsStore->clear();
            return;
        }

        LayoutEngine engine;
        QVector<ClassLayout> layouts = engine.run(code, classes);

        if (layouts.isEmpty()) {
            statusLabel->setText("状态：解析到了类，但探针编译或运行失败");
            summaryEdit->setPlainText(analysisFailureMessage("", includeHints, engine.lastError()));
            tabs->setCurrentIndex(0);

            classList->clear();
            optimizationEdit->clear();
            fieldTable->clear();
            baseTable->clear();
            functionTable->clear();
            memoryScene->clear();
            vtableScene->clear();
            layoutsStore->clear();
            return;
        }

        *layoutsStore = layouts;

        classList->clear();
        for (const ClassLayout &layoutInfo : layouts) {
            QString itemText = QString("%1\nSize: %2 bytes")
            .arg(QString::fromStdString(layoutInfo.className))
                .arg(layoutInfo.totalSize);
            QListWidgetItem *item = new QListWidgetItem(itemText);
            item->setSizeHint(QSize(130, 58));
            classList->addItem(item);
        }

        if (!layouts.isEmpty()) {
            int selectedRow = 0;
            for (int i = 0; i < layouts.size(); ++i) {
                if (layouts[i].className == "Dog") {
                    selectedRow = i;
                    break;
                }
            }
            classList->setCurrentRow(selectedRow);
            showClass(selectedRow);
        }

        statusLabel->setText(QString("状态：分析成功，共解析到 %1 个类").arg(layouts.size()));
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}
