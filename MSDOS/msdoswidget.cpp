/* Copyright (c) 2017-2025 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "msdoswidget.h"

#include "ui_msdoswidget.h"

MSDOSWidget::MSDOSWidget(QWidget *pParent) : FormatWidget(pParent), ui(new Ui::MSDOSWidget)
{
    ui->setupUi(this);

    XOptions::adjustToolButton(ui->toolButtonReload, XOptions::ICONTYPE_RELOAD);
    XOptions::adjustToolButton(ui->toolButtonNext, XOptions::ICONTYPE_FORWARD, Qt::ToolButtonIconOnly);
    XOptions::adjustToolButton(ui->toolButtonPrev, XOptions::ICONTYPE_BACKWARD, Qt::ToolButtonIconOnly);

    ui->toolButtonReload->setToolTip(tr("Reload"));
    ui->toolButtonNext->setToolTip(tr("Next visited"));
    ui->toolButtonPrev->setToolTip(tr("Previous visited"));
    ui->checkBoxReadonly->setToolTip(tr("Readonly"));

    memset(g_subDevice, 0, sizeof g_subDevice);

    initWidget();
}

MSDOSWidget::MSDOSWidget(QIODevice *pDevice, FW_DEF::OPTIONS options, QWidget *pParent) : MSDOSWidget(pParent)
{
    MSDOSWidget::setData(pDevice, options, 0, 0, 0);
    MSDOSWidget::reload();
}

MSDOSWidget::~MSDOSWidget()
{
    delete ui;
}

void MSDOSWidget::clear()
{
    MSDOSWidget::reset();

    memset(g_lineEdit_DOS_HEADER, 0, sizeof g_lineEdit_DOS_HEADER);
    memset(g_comboBox, 0, sizeof g_comboBox);

    _deleteSubdevices(g_subDevice, (sizeof g_subDevice) / (sizeof(SubDevice *)));

    resetWidget();

    ui->checkBoxReadonly->setChecked(true);

    ui->treeWidgetNavi->clear();
}

void MSDOSWidget::cleanup()
{
    MSDOSWidget::clear();
}

void MSDOSWidget::reload()
{
    MSDOSWidget::clear();

    ui->checkBoxReadonly->setEnabled(!isReadonly());

    XMSDOS msdos(getDevice(), getOptions().bIsImage, getOptions().nImageBase);

    if (msdos.isValid()) {
        setFileType(msdos.getFileType());

        QTreeWidgetItem *pItemInfo = createNewItem(SMSDOS::TYPE_INFO, tr("Info"), XOptions::ICONTYPE_INFO);
        ui->treeWidgetNavi->addTopLevelItem(pItemInfo);
        pItemInfo->addChild(createNewItem(SMSDOS::TYPE_NFDSCAN, "Nauz File Detector (NFD)", XOptions::ICONTYPE_NFD));
        pItemInfo->addChild(createNewItem(SMSDOS::TYPE_DIESCAN, "Detect It Easy (DiE)", XOptions::ICONTYPE_DIE));
#ifdef USE_YARA
        pItemInfo->addChild(createNewItem(SMSDOS::TYPE_YARASCAN, "Yara rules", XOptions::ICONTYPE_YARA));
#endif
        pItemInfo->addChild(createNewItem(SMSDOS::TYPE_VIRUSTOTAL, "VirusTotal", XOptions::ICONTYPE_VIRUSTOTAL));

        ui->treeWidgetNavi->addTopLevelItem(createNewItem(SMSDOS::TYPE_VISUALIZATION, tr("Visualization"), XOptions::ICONTYPE_VISUALIZATION));
        ui->treeWidgetNavi->addTopLevelItem(createNewItem(SMSDOS::TYPE_HEX, tr("Hex"), XOptions::ICONTYPE_HEX));
        ui->treeWidgetNavi->addTopLevelItem(createNewItem(SMSDOS::TYPE_DISASM, tr("Disasm"), XOptions::ICONTYPE_DISASM));
        ui->treeWidgetNavi->addTopLevelItem(createNewItem(SMSDOS::TYPE_HASH, tr("Hash"), XOptions::ICONTYPE_HASH));
        ui->treeWidgetNavi->addTopLevelItem(createNewItem(SMSDOS::TYPE_STRINGS, tr("Strings"), XOptions::ICONTYPE_STRING));
        ui->treeWidgetNavi->addTopLevelItem(createNewItem(SMSDOS::TYPE_SIGNATURES, tr("Signatures"), XOptions::ICONTYPE_SIGNATURE));
        ui->treeWidgetNavi->addTopLevelItem(createNewItem(SMSDOS::TYPE_MEMORYMAP, tr("Memory map"), XOptions::ICONTYPE_MEMORYMAP));
        ui->treeWidgetNavi->addTopLevelItem(createNewItem(SMSDOS::TYPE_ENTROPY, tr("Entropy"), XOptions::ICONTYPE_ENTROPY));
        ui->treeWidgetNavi->addTopLevelItem(createNewItem(SMSDOS::TYPE_EXTRACTOR, tr("Extractor"), XOptions::ICONTYPE_EXTRACTOR));
        ui->treeWidgetNavi->addTopLevelItem(createNewItem(SMSDOS::TYPE_SEARCH, tr("Search"), XOptions::ICONTYPE_SEARCH));
        ui->treeWidgetNavi->addTopLevelItem(createNewItem(SMSDOS::TYPE_DOS_HEADER, "DOS_HEADER", XOptions::ICONTYPE_HEADER));

        if (msdos.isOverlayPresent()) {
            ui->treeWidgetNavi->addTopLevelItem(createNewItem(SMSDOS::TYPE_OVERLAY, tr("Overlay"), XOptions::ICONTYPE_OVERLAY));
        }

        ui->treeWidgetNavi->expandAll();

        setDisasmInitAddress(msdos.getEntryPointAddress());  // Optimize

        setTreeItem(ui->treeWidgetNavi, getOptions().nStartType);
    }
}

FormatWidget::SV MSDOSWidget::_setValue(QVariant vValue, qint32 nStype, qint32 nNdata, qint32 nVtype, qint32 nPosition, qint64 nOffset)
{
    Q_UNUSED(nVtype)
    Q_UNUSED(nPosition)
    Q_UNUSED(nOffset)

    SV result = SV_NONE;

    blockSignals(true);

    quint64 nValue = vValue.toULongLong();

    if (getDevice()->isWritable()) {
        XMSDOS msdos(getDevice(), getOptions().bIsImage, getOptions().nImageBase);

        if (msdos.isValid()) {
            switch (nStype) {
                case SMSDOS::TYPE_DOS_HEADER:
                    switch (nNdata) {
                        case N_DOS_HEADER::e_magic: g_comboBox[CB_DOS_HEADER_e_magic]->setValue(nValue); break;
                    }
                    break;
            }

            switch (nStype) {
                case SMSDOS::TYPE_DOS_HEADER:
                    switch (nNdata) {
                        case N_DOS_HEADER::e_magic: msdos.set_e_magic((quint16)nValue); break;
                        case N_DOS_HEADER::e_cblp: msdos.set_e_cblp((quint16)nValue); break;
                        case N_DOS_HEADER::e_cp: msdos.set_e_cp((quint16)nValue); break;
                        case N_DOS_HEADER::e_crlc: msdos.set_e_crlc((quint16)nValue); break;
                        case N_DOS_HEADER::e_cparhdr: msdos.set_e_cparhdr((quint16)nValue); break;
                        case N_DOS_HEADER::e_minalloc: msdos.set_e_minalloc((quint16)nValue); break;
                        case N_DOS_HEADER::e_maxalloc: msdos.set_e_maxalloc((quint16)nValue); break;
                        case N_DOS_HEADER::e_ss: msdos.set_e_ss((quint16)nValue); break;
                        case N_DOS_HEADER::e_sp: msdos.set_e_sp((quint16)nValue); break;
                        case N_DOS_HEADER::e_csum: msdos.set_e_csum((quint16)nValue); break;
                        case N_DOS_HEADER::e_ip: msdos.set_e_ip((quint16)nValue); break;
                        case N_DOS_HEADER::e_cs: msdos.set_e_cs((quint16)nValue); break;
                        case N_DOS_HEADER::e_lfarlc: msdos.set_e_lfarlc((quint16)nValue); break;
                        case N_DOS_HEADER::e_ovno: msdos.set_e_ovno((quint16)nValue); break;
                    }

                    ui->widgetHex_DOS_HEADER->reload();

                    break;
            }

            result = SV_EDITED;
        }
    }

    blockSignals(false);

    return result;
}

void MSDOSWidget::setReadonly(bool bState)
{
    if (ui->checkBoxReadonly->isChecked() != bState) {
        const bool bBlocked1 = ui->checkBoxReadonly->blockSignals(true);
        ui->checkBoxReadonly->setChecked(bState);
        ui->checkBoxReadonly->blockSignals(bBlocked1);
    }

    setLineEditsReadOnly(g_lineEdit_DOS_HEADER, N_DOS_HEADER::__data_size, bState);

    setComboBoxesReadOnly(g_comboBox, __CB_size, bState);

    ui->widgetHex->setReadonly(bState);
    ui->widgetDisasm->setReadonly(bState);
    ui->widgetStrings->setReadonly(bState);
}

void MSDOSWidget::blockSignals(bool bState)
{
    _blockSignals((QObject **)g_lineEdit_DOS_HEADER, N_DOS_HEADER::__data_size, bState);

    _blockSignals((QObject **)g_comboBox, __CB_size, bState);
}

void MSDOSWidget::adjustHeaderTable(qint32 nType, QTableWidget *pTableWidget)
{
    FormatWidget::adjustHeaderTable(nType, pTableWidget);
}

QString MSDOSWidget::typeIdToString(qint32 nType)
{
    Q_UNUSED(nType)

    QString sResult;

    // TODO

    return sResult;
}

void MSDOSWidget::_showInDisasmWindowAddress(qint64 nAddress)
{
    setTreeItem(ui->treeWidgetNavi, SMSDOS::TYPE_DISASM);
    ui->widgetDisasm->setLocation(nAddress, XBinary::LT_ADDRESS, 0);
}

void MSDOSWidget::_showInDisasmWindowOffset(qint64 nOffset)
{
    setTreeItem(ui->treeWidgetNavi, SMSDOS::TYPE_DISASM);
    ui->widgetDisasm->setLocation(nOffset, XBinary::LT_OFFSET, 0);
}

void MSDOSWidget::_showInMemoryMapWindowOffset(qint64 nOffset)
{
    setTreeItem(ui->treeWidgetNavi, SMSDOS::TYPE_MEMORYMAP);
    ui->widgetMemoryMap->goToOffset(nOffset);
}

void MSDOSWidget::_showInHexWindow(qint64 nOffset, qint64 nSize)
{
    setTreeItem(ui->treeWidgetNavi, SMSDOS::TYPE_HEX);
    ui->widgetHex->setSelection(nOffset, nSize);
}

void MSDOSWidget::_findValue(quint64 nValue, XBinary::ENDIAN endian)
{
    setTreeItem(ui->treeWidgetNavi, SMSDOS::TYPE_SEARCH);
    ui->widgetSearch->findValue(nValue, endian);
}

void MSDOSWidget::reloadData(bool bSaveSelection)
{
    qint32 nType = ui->treeWidgetNavi->currentItem()->data(0, Qt::UserRole + FW_DEF::SECTION_DATA_TYPE).toInt();
    //    qint64
    //    nDataOffset=ui->treeWidgetNavi->currentItem()->data(0,Qt::UserRole+FW_DEF::SECTION_DATA_OFFSET).toLongLong();
    //    qint64
    //    nDataSize=ui->treeWidgetNavi->currentItem()->data(0,Qt::UserRole+FW_DEF::SECTION_DATA_SIZE).toLongLong();

    QString sInit = getInitString(ui->treeWidgetNavi->currentItem());

    ui->stackedWidgetInfo->setCurrentIndex(nType);

    XMSDOS msdos(getDevice(), getOptions().bIsImage, getOptions().nImageBase);

    if (msdos.isValid()) {
        if (nType == SMSDOS::TYPE_INFO) {
            if (!isInitPresent(sInit)) {
                ui->widgetInfo->setData(getDevice(), msdos.getFileType(), "Info", true);
            }
        } else if (nType == SMSDOS::TYPE_VISUALIZATION) {
            if (!isInitPresent(sInit)) {
                ui->widgetVisualization->setData(getDevice(), msdos.getFileType(), true);
            }
        } else if (nType == SMSDOS::TYPE_VIRUSTOTAL) {
            if (!isInitPresent(sInit)) {
                ui->widgetVirusTotal->setData(getDevice());
            }
        } else if (nType == SMSDOS::TYPE_HEX) {
            ui->widgetHex->setWidgetFocus();
            if (!isInitPresent(sInit)) {
                XHexViewWidget::OPTIONS options = {};
                options.bMenu_Disasm = true;
                options.bMenu_MemoryMap = true;
                options.fileType = msdos.getFileType();

                if (bSaveSelection) {
                    options.nStartSelectionOffset = -1;
                }

                ui->widgetHex->setXInfoDB(getXInfoDB());
                ui->widgetHex->setData(getDevice(), options);
                //                ui->widgetHex->setBackupFileName(getOptions().sBackupFileName);
                //                ui->widgetHex->enableReadOnly(false);
            }
        } else if (nType == SMSDOS::TYPE_DISASM) {
            ui->widgetDisasm->setWidgetFocus();
            if (!isInitPresent(sInit)) {
                XMultiDisasmWidget::OPTIONS options = {};
                options.fileType = msdos.getFileType();
                options.nInitAddress = getDisasmInitAddress();
                options.bMenu_Hex = true;
                ui->widgetDisasm->setXInfoDB(getXInfoDB());
                ui->widgetDisasm->setData(getDevice(), options);

                setDisasmInitAddress(-1);
            }
        } else if (nType == SMSDOS::TYPE_HASH) {
            if (!isInitPresent(sInit)) {
                ui->widgetHash->setData(getDevice(), msdos.getFileType(), 0, -1, true);
            }
        } else if (nType == SMSDOS::TYPE_STRINGS) {
            if (!isInitPresent(sInit)) {
                SearchStringsWidget::OPTIONS stringsOptions = {};
                stringsOptions.bMenu_Hex = true;
                stringsOptions.bMenu_Demangle = true;
                stringsOptions.bAnsi = true;
                // stringsOptions.bUTF8 = false;
                stringsOptions.bUnicode = true;
                stringsOptions.bNullTerminated = false;

                ui->widgetStrings->setData(getDevice(), msdos.getFileType(), stringsOptions, true);
            }
        } else if (nType == SMSDOS::TYPE_SIGNATURES) {
            if (!isInitPresent(sInit)) {
                SearchSignaturesWidget::OPTIONS signaturesOptions = {};
                signaturesOptions.bMenu_Hex = true;
                signaturesOptions.fileType = msdos.getFileType();

                ui->widgetSignatures->setData(getDevice(), signaturesOptions, false);
            }
        } else if (nType == SMSDOS::TYPE_MEMORYMAP) {
            if (!isInitPresent(sInit)) {
                XMemoryMapWidget::OPTIONS options = {};
                options.fileType = msdos.getFileType();
                options.bIsSearchEnable = true;

                ui->widgetMemoryMap->setData(getDevice(), options, getXInfoDB());
            }
        } else if (nType == SMSDOS::TYPE_ENTROPY) {
            if (!isInitPresent(sInit)) {
                ui->widgetEntropy->setData(getDevice(), 0, getDevice()->size(), msdos.getFileType(), true);
            }
        } else if (nType == SMSDOS::TYPE_NFDSCAN) {
            if (!isInitPresent(sInit)) {
                ui->widgetHeuristicScan->setData(getDevice(), true, msdos.getFileType());
            }
        } else if (nType == SMSDOS::TYPE_DIESCAN) {
            if (!isInitPresent(sInit)) {
                ui->widgetDIEScan->setData(getDevice(), true, msdos.getFileType());
            }
#ifdef USE_YARA
        } else if (nType == SMSDOS::TYPE_YARASCAN) {
            if (!isInitPresent(sInit)) {
                ui->widgetYARAScan->setData(XBinary::getDeviceFileName(getDevice()), true);
            }
#endif
        } else if (nType == SMSDOS::TYPE_EXTRACTOR) {
            if (!isInitPresent(sInit)) {
                XExtractor::OPTIONS extractorOptions = XExtractor::getDefaultOptions();
                extractorOptions.fileType = msdos.getFileType();
                extractorOptions.bMenu_Hex = true;

                ui->widgetExtractor->setData(getDevice(), getXInfoDB(), extractorOptions, true);
            }
        } else if (nType == SMSDOS::TYPE_SEARCH) {
            if (!isInitPresent(sInit)) {
                SearchValuesWidget::OPTIONS options = {};
                options.fileType = msdos.getFileType();
                options.bMenu_Hex = true;
                options.bMenu_Disasm = true;

                ui->widgetSearch->setData(getDevice(), options);
            }
        } else if (nType == SMSDOS::TYPE_DOS_HEADER) {
            if (!isInitPresent(sInit)) {
                createHeaderTable(SMSDOS::TYPE_DOS_HEADER, ui->tableWidget_DOS_HEADER, N_DOS_HEADER::records, g_lineEdit_DOS_HEADER, N_DOS_HEADER::__data_size, 0);
                g_comboBox[CB_DOS_HEADER_e_magic] =
                    createComboBox(ui->tableWidget_DOS_HEADER, XMSDOS::getImageMagicsS(), SMSDOS::TYPE_DOS_HEADER, N_DOS_HEADER::e_magic, XComboBoxEx::CBTYPE_LIST);

                blockSignals(true);

                XMSDOS_DEF::IMAGE_DOS_HEADEREX msdosheaderex = msdos.getDosHeaderEx();

                g_lineEdit_DOS_HEADER[N_DOS_HEADER::e_magic]->setValue_uint16(msdosheaderex.e_magic);
                g_lineEdit_DOS_HEADER[N_DOS_HEADER::e_cblp]->setValue_uint16(msdosheaderex.e_cblp);
                g_lineEdit_DOS_HEADER[N_DOS_HEADER::e_cp]->setValue_uint16(msdosheaderex.e_cp);
                g_lineEdit_DOS_HEADER[N_DOS_HEADER::e_crlc]->setValue_uint16(msdosheaderex.e_crlc);
                g_lineEdit_DOS_HEADER[N_DOS_HEADER::e_cparhdr]->setValue_uint16(msdosheaderex.e_cparhdr);
                g_lineEdit_DOS_HEADER[N_DOS_HEADER::e_minalloc]->setValue_uint16(msdosheaderex.e_minalloc);
                g_lineEdit_DOS_HEADER[N_DOS_HEADER::e_maxalloc]->setValue_uint16(msdosheaderex.e_maxalloc);
                g_lineEdit_DOS_HEADER[N_DOS_HEADER::e_ss]->setValue_uint16(msdosheaderex.e_ss);
                g_lineEdit_DOS_HEADER[N_DOS_HEADER::e_sp]->setValue_uint16(msdosheaderex.e_sp);
                g_lineEdit_DOS_HEADER[N_DOS_HEADER::e_csum]->setValue_uint16(msdosheaderex.e_csum);
                g_lineEdit_DOS_HEADER[N_DOS_HEADER::e_ip]->setValue_uint16(msdosheaderex.e_ip);
                g_lineEdit_DOS_HEADER[N_DOS_HEADER::e_cs]->setValue_uint16(msdosheaderex.e_cs);
                g_lineEdit_DOS_HEADER[N_DOS_HEADER::e_lfarlc]->setValue_uint16(msdosheaderex.e_lfarlc);
                g_lineEdit_DOS_HEADER[N_DOS_HEADER::e_ovno]->setValue_uint16(msdosheaderex.e_ovno);

                g_comboBox[CB_DOS_HEADER_e_magic]->setValue(msdosheaderex.e_magic);

                qint64 nOffset = msdos.getDosHeaderExOffset();  // Ex!
                qint64 nSize = msdos.getDosHeaderExSize();

                loadHexSubdevice(nOffset, nSize, nOffset, &g_subDevice[SMSDOS::TYPE_DOS_HEADER], ui->widgetHex_DOS_HEADER);

                blockSignals(false);
            }
        } else if (nType == SMSDOS::TYPE_OVERLAY) {
            if (!isInitPresent(sInit)) {
                qint64 nOverLayOffset = msdos.getOverlayOffset();
                qint64 nOverlaySize = msdos.getOverlaySize();

                loadHexSubdevice(nOverLayOffset, nOverlaySize, nOverLayOffset, &g_subDevice[SMSDOS::TYPE_OVERLAY], ui->widgetHex_OVERLAY);
            }
        }

        setReadonly(ui->checkBoxReadonly->isChecked());
    }

    addInit(sInit);
}

void MSDOSWidget::_widgetValueChanged(QVariant vValue)
{
    QWidget *pWidget = qobject_cast<QWidget *>(sender());
    qint32 nStype = pWidget->property("STYPE").toInt();
    qint32 nNdata = pWidget->property("NDATA").toInt();

    quint64 nValue = vValue.toULongLong();

    switch (nStype) {
        case SMSDOS::TYPE_DOS_HEADER:
            switch (nNdata) {
                case N_DOS_HEADER::e_magic: g_lineEdit_DOS_HEADER[N_DOS_HEADER::e_magic]->setValue_uint16((quint16)nValue); break;
            }

            break;
    }
}

void MSDOSWidget::on_treeWidgetNavi_currentItemChanged(QTreeWidgetItem *pItemCurrent, QTreeWidgetItem *pItemPrevious)
{
    Q_UNUSED(pItemPrevious)

    if (pItemCurrent) {
        reloadData(false);
        addPage(pItemCurrent);
        ui->toolButtonPrev->setEnabled(isPrevPageAvailable());
        ui->toolButtonNext->setEnabled(isNextPageAvailable());
    }
}

void MSDOSWidget::on_checkBoxReadonly_toggled(bool bChecked)
{
    setReadonly(bChecked);
}

void MSDOSWidget::on_toolButtonReload_clicked()
{
    ui->toolButtonReload->setEnabled(false);
    reload();

    QTimer::singleShot(1000, this, SLOT(enableButton()));
}

void MSDOSWidget::enableButton()
{
    ui->toolButtonReload->setEnabled(true);
}

void MSDOSWidget::on_tableWidget_DOS_HEADER_currentCellChanged(int nCurrentRow, int nCurrentColumn, int nPreviousRow, int nPreviousColumn)
{
    Q_UNUSED(nCurrentRow);
    Q_UNUSED(nCurrentColumn);
    Q_UNUSED(nPreviousRow);
    Q_UNUSED(nPreviousColumn);

    setHeaderTableSelection(ui->widgetHex_DOS_HEADER, ui->tableWidget_DOS_HEADER);
}

void MSDOSWidget::on_toolButtonPrev_clicked()
{
    setAddPageEnabled(false);
    ui->treeWidgetNavi->setCurrentItem(getPrevPage());
    setAddPageEnabled(true);
}

void MSDOSWidget::on_toolButtonNext_clicked()
{
    setAddPageEnabled(false);
    ui->treeWidgetNavi->setCurrentItem(getNextPage());
    setAddPageEnabled(true);
}

void MSDOSWidget::on_pushButtonDump_Overlay_clicked()
{
    XMSDOS msdos(getDevice(), getOptions().bIsImage, getOptions().nImageBase);

    if (msdos.isValid()) {
        qint64 nOffset = msdos.getOverlayOffset();
        qint64 nSize = msdos.getOverlaySize();

        dumpRegion(nOffset, nSize, tr("Overlay"));
    }
}
