#include "slingroad.h"
#include "ui_slingroad.h"
#include "ui_interface.h"
#include "market.h"
#include "key.h"
#include "base58.h"
#include "marketlistingdetailsdialog.h"
#include "init.h"
#include "wallet.h"

#include <QClipboard>
#include <QMessageBox>


SlingRoad::SlingRoad(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SlingRoad)
{
    ui->setupUi(this);
    subscribeToCoreSignals();

    ui->slingRoadHeaderLabel->setPixmap(QPixmap(":/images/slingroad"));
    updateCategories();

    ui->buyButton->setEnabled(false);
    ui->viewDetailsButton->setEnabled(false);
    ui->copyAddressButton->setEnabled(false);
    lastPrice = 0;
}

SlingRoad::~SlingRoad()
{
    unsubscribeFromCoreSignals();
    delete ui;
}

void SlingRoad::on_categoriesListWidget_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
    // get the selected category and filter the listings
    std::string cat = ui->categoriesListWidget->currentItem()->text().toStdString();
    std::set<CSignedMarketListing> listings = mapListingsByCategory[cat];
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);

    LOCK(cs_markets);
    BOOST_FOREACH(CSignedMarketListing p, listings)
    {
	// only show if no buy requests
	bool bFound = false;
	BOOST_FOREACH(PAIRTYPE(const uint256, CBuyRequest)& b, mapBuyRequests)
	{
	    if(b.second.listingId == p.GetHash())
	    {
		if(b.second.nStatus != LISTED || b.second.nStatus != BUY_REQUESTED)
		{
		    bFound = true;	
		}
	    }
	}

	if(bFound)
	    continue; // go to next item, this one someone is already buying

 	// until, vendor, price, title
	QTableWidgetItem* untilItem = new QTableWidgetItem(QString::fromStdString(DateTimeStrFormat(p.listing.nCreated + (7 * 24 * 60 * 60))));
        QTableWidgetItem* vendorItem = new QTableWidgetItem(QString::fromStdString(CBitcoinAddress(p.listing.sellerKey.GetID()).ToString()));
        QTableWidgetItem* priceItem = new QTableWidgetItem(QString::number(p.listing.nPrice / COIN, 'f', 8));
	QTableWidgetItem* btcPriceItem = new QTableWidgetItem(QString::number( (p.listing.nPrice/COIN) * lastPrice, 'f', 8));
        QTableWidgetItem* titleItem = new QTableWidgetItem(QString::fromStdString(p.listing.sTitle));
	QTableWidgetItem* idItem = new QTableWidgetItem(QString::fromStdString(p.GetHash().ToString()));
        ui->tableWidget->insertRow(0);
        ui->tableWidget->setItem(0, 0, untilItem);
        ui->tableWidget->setItem(0, 1, vendorItem);
        ui->tableWidget->setItem(0, 2, priceItem);
	ui->tableWidget->setItem(0, 3, btcPriceItem);
        ui->tableWidget->setItem(0, 4, titleItem);
        ui->tableWidget->setItem(0, 5, idItem);
    }

}



void SlingRoad::updateCategories()
{
    ui->categoriesListWidget->clear();
    BOOST_FOREACH(PAIRTYPE(const std::string, std::set<CSignedMarketListing>)& p, mapListingsByCategory)
    {
	ui->categoriesListWidget->addItem(QString::fromStdString(p.first));
    }
}

void SlingRoad::updateCategory(QString category)
{
    if(category == "")
	return;

    // if the category list doesn't contain this one, add it
    bool bFound = false;
    for(int i = 0; i < ui->categoriesListWidget->count(); ++i)
    {
        QListWidgetItem* item = ui->categoriesListWidget->item(i);
        if(item->text() == category)
 	{
	    bFound = true;
	    break;
	}
    }

    if(!bFound)
    {
	ui->categoriesListWidget->addItem(category);
    }
}

static void NotifyMarketCategory(SlingRoad *page, std::string category)
{
    QString cat = QString::fromStdString(category);
    QMetaObject::invokeMethod(page, "updateCategory", Qt::QueuedConnection,
                              Q_ARG(QString, cat));
}

static void NotifyListingCancelled(SlingRoad *page)
{
    
}

void SlingRoad::subscribeToCoreSignals()
{
    // Connect signals to core
    uiInterface.NotifyMarketCategory.connect(boost::bind(&NotifyMarketCategory, this, _1));
    uiInterface.NotifyListingCancelled.connect(boost::bind(&NotifyListingCancelled, this));
}

void SlingRoad::unsubscribeFromCoreSignals()
{
    // Disconnect signals from core
    uiInterface.NotifyMarketCategory.disconnect(boost::bind(&NotifyMarketCategory, this, _1));
    uiInterface.NotifyListingCancelled.disconnect(boost::bind(&NotifyListingCancelled, this));
}

void SlingRoad::on_tableWidget_itemSelectionChanged()
{
    if(ui->tableWidget->selectedItems().count() > 0)
    {
        ui->buyButton->setEnabled(true);
        ui->viewDetailsButton->setEnabled(true);
        ui->copyAddressButton->setEnabled(true);
    }
}

void SlingRoad::on_buyButton_clicked()
{
    QItemSelectionModel* selectionModel = ui->tableWidget->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QModelIndex index = selected.at(0);
    int r = index.row();
    std::string id = ui->tableWidget->item(r, 4)->text().toStdString();
    uint256 idHash = uint256(id);

    // ask the user if they really want to put in a buy request
    QMessageBox::StandardButton reply;
      reply = QMessageBox::question(this, "Request Buy", "Are you sure you want to send a buy request for this item?",
                                QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) 
    {
        CBuyRequest buyRequest;
	buyRequest.buyerKey = pwalletMain->GenerateNewKey();
	buyRequest.listingId = idHash;
	buyRequest.nStatus = BUY_REQUESTED;
	buyRequest.requestId = buyRequest.GetHash();
	SignBuyRequest(buyRequest, buyRequest.vchSig);
	ReceiveBuyRequest(buyRequest);
	buyRequest.BroadcastToAll();	
	uiInterface.NotifyBuyRequest(buyRequest);
    } 
  
}

void SlingRoad::on_viewDetailsButton_clicked()
{
    QItemSelectionModel* selectionModel = ui->tableWidget->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QModelIndex index = selected.at(0);
    int r = index.row();
    std::string id = ui->tableWidget->item(r, 4)->text().toStdString();
    uint256 idHash = uint256(id);

    // show the detailed item listing dialog
    MarketListingDetailsDialog* md = new MarketListingDetailsDialog(this, idHash);
    md->exec();
}

void SlingRoad::on_copyAddressButton_clicked()
{
    QItemSelectionModel* selectionModel = ui->tableWidget->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    if(selected.count() == 0)
        return;

    QModelIndex index = selected.at(0);
    int r = index.row();
    std::string address = ui->tableWidget->item(r, 1)->text().toStdString();
    QApplication::clipboard()->setText(QString::fromStdString(address));
}
