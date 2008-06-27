#include "collectionview.h"
#include "database.h"
#include "tagger.h"
#include "indicator.h"

/************************
 *
 *   CollectionFiller
 *
 ************************/

CollectionFiller::CollectionFiller(QList<QUrl> _urls, ListMode _mode, QString _attrname, QObject * parent) 
: QThread(parent), urls(_urls), mode(_mode), attrname(_attrname)
{
	cancel = false;
	connect(&Indicator::Self(), SIGNAL(userStop()), this, SLOT(cancelEvent()));
}

CollectionFiller::~CollectionFiller()
{
	disconnect(&Indicator::Self(), SIGNAL(userStop()), this, SLOT(cancel()));
}

void CollectionFiller::run()
{
	int taskID = Indicator::Self().addTask("Collect music");
	foreach(QUrl url, urls) {
		proceed(url.toLocalFile());
	}
	Indicator::Self().delTask(taskID);
}

void CollectionFiller::proceed(QString path)
{
	QDir dir;
	if(dir.cd(path)) {
		foreach(QString file, dir.entryList()) {
			if(file == "." || file == "..") continue;
			proceed(dir.filePath(file));
		}
	} else {
		if(attrname.length()) {
			QString title, artist, album, comment, genre, length;
			int track, year, rating;
			bool exist = Database::Self().GetTags(path, title, artist, album, comment, genre, track, year, rating, length);
			switch(mode) {
				case M_ALBUM:
					Tagger::updateAlbum(path, attrname);
					album = attrname;
					break;
				case M_ARTIST:
					Tagger::updateArtist(path, attrname);
					artist = attrname;
					break;
				case M_GENRE:
					Tagger::updateGenre(path, attrname);
					genre = attrname;
					break;
				case M_MARK:
					Database::Self().SetMark(path, attrname);
					return;
			}
			if(exist) Database::Self().SetTags(path, title, artist, album, comment, genre, track, year, rating); 
			else Database::Self().AddFile(path);
		} else Database::Self().AddFile(path);
	}
}

void CollectionFiller::cancelEvent()
{
	cancel = true;
}

/************************
 * 
 *  CollectionModel
 * 
 ************************/

CollectionModel::CollectionModel(QObject *parent): QStandardItemModel(parent)
{
	setSupportedDragActions(Qt::CopyAction | Qt::MoveAction);
	searchPattern = "";
}

CollectionModel::~CollectionModel()
{
	
}

bool CollectionModel::setData ( const QModelIndex & index, const QVariant & value, int role )
{
	QString oldvalue = data(index, role).toString(),
		newvalue = value.toString();
	QList<QString> files;
	if(oldvalue != newvalue) switch(mode) {
	case M_ARTIST:
		files = Database::Self().Songs(&oldvalue);
		foreach(QString file, files) Tagger::updateArtist(file, newvalue);
		Database::Self().RenameArtist(oldvalue, newvalue);
		break;
	case M_ALBUM:
		files = Database::Self().Songs(0, &oldvalue);
		foreach(QString file, files) Tagger::updateAlbum(file, newvalue);
		Database::Self().RenameAlbum(oldvalue, newvalue);
		break;
	case M_GENRE:
		files = Database::Self().Songs(0, 0, &oldvalue);
		foreach(QString file, files) Tagger::updateGenre(file, newvalue);
		Database::Self().RenameGenre(oldvalue, newvalue);
		break;
	case M_MARK:
		Database::Self().RenameMark(oldvalue, newvalue);
		break;
	}
	//QMessageBox::information(0, "", value.toString());
	return QStandardItemModel::setData(index, value, role);
}

QStringList CollectionModel::mimeTypes() const
{
	QStringList list;
	list << "text/uri-list";
	return list;
}

QMimeData *CollectionModel::mimeData( const QModelIndexList & indexes ) const
{
    QMimeData *mimeData = QStandardItemModel::mimeData(indexes);
    QList<QUrl> list;
    foreach(QModelIndex ind, indexes) {
		list += SelectByItem(ind);
    }
    //QMessageBox::information(this, "", QString::number(list.count()));
    mimeData->setUrls(list);
    /*QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << list.count();
    foreach(QModelIndex ind, this->selectedIndexes()) {
        dataStream << ind.column();
        dataStream << model.data(ind, Qt::DisplayRole).toString();
    }
    mimeData->setData("application/x-playlist", itemData);*/
    return mimeData;
}

// select files
QList<QUrl> CollectionModel::SelectByItem(QModelIndex i) const
{
	QString s = data(i).toString();
	QList<QString> res;
	switch(mode) {
	case M_ARTIST:
		res = Database::Self().Songs(&s, 0, 0, 0);
		break;
	case M_ALBUM:
		res = Database::Self().Songs(0, &s, 0, 0);
		break;
	case M_GENRE:
		res = Database::Self().Songs(0, 0, &s, 0);
		break;
	case M_MARK:
		res = Database::Self().Songs(0, 0, 0, &s);
		break;
	}
	QList<QUrl> urls;
	foreach(QString f, res) {
		urls << QUrl::fromLocalFile(f);
	}
	//QUrl::fromLocalFile(model.data(ind, Qt::UserRole).toString()));
	return urls;
}

bool CollectionModel::dropMimeData ( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent )
{
    if (data->hasUrls()) {
    	QString attrname("");
    	if(parent.isValid()) attrname = this->data(parent, Qt::DisplayRole).toString();
    	QList<QUrl> urls = data->urls();
    	CollectionFiller * cf = new CollectionFiller(urls, mode, attrname);
    	connect(cf, SIGNAL(finished()), this, SLOT(update()));
    	cf->start();
    	//foreach(QUrl url, urls) {
    		//Database::Self().AddFile(url.toLocalFile());
    	//}
        //updateMode(mode);
        return true;
    }
    return false;
}

void CollectionModel::update()
{
	updateMode(mode);
}

Qt::DropActions CollectionModel::supportedDropActions() const
{
	return Qt::CopyAction | Qt::MoveAction;
}

void CollectionModel::updateMode(ListMode m)
{
	mode = m;
	clear();
	QMap<QString, int> data;
	QIcon icon;
	QString stat("Collection - ");
	switch(m) {
	case M_ARTIST:
		if(searchPattern.length()) data = Database::Self().Artists(&searchPattern);
		else data = Database::Self().Artists();
		//icon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
		icon = QIcon(":/icons/def_artist.png");
		stat += QString::number(data.count())+QString(" ")+tr("artists");
		break;
	case M_ALBUM:
		if(searchPattern.length()) data = Database::Self().Albums(&searchPattern);
		else data = Database::Self().Albums();
		//icon = QApplication::style()->standardIcon(QStyle::SP_DriveCDIcon);
		icon = QIcon(":/icons/def_album.png");
		stat += QString::number(data.count())+QString(" ")+tr("albums");
		break;
	case M_GENRE:
		if(searchPattern.length()) data = Database::Self().Genres(&searchPattern);
		else data = Database::Self().Genres();
		//icon = QApplication::style()->standardIcon(QStyle::SP_DirIcon);
		icon = QIcon(":/icons/def_genre.png");
		stat += QString::number(data.count())+QString(" ")+tr("genres");
		break;
	case M_MARK:
		if(searchPattern.length()) data = Database::Self().Marks(&searchPattern);
		else data = Database::Self().Marks();
		//icon = QApplication::style()->standardIcon(QStyle::SP_DirIcon);
		icon = QIcon(":/icons/def_mark.png");
		stat += QString::number(data.count())+QString(" ")+tr("marks");
		break;
	}
	QString tt("");
	QStandardItem *i;
	for(QMap<QString, int>::const_iterator it = data.constBegin(); it != data.constEnd(); ++it) {
		i = new QStandardItem(icon, it.key());
		tt = QString::number(it.value()).append(QString(" ")+tr("songs"));
		i->setToolTip(tt); 
		appendRow(i);
	}
	emit status(stat);
}


/***********************
 * 
 *  CollectionView 
 * 
 ***********************/

CollectionView::CollectionView(QWidget *parent)
    : QListView(parent)
{
	ui.setupUi(this);
	setModel(&model);
	//setViewMode(QListView::IconMode);
	setSelectionMode(QAbstractItemView::ExtendedSelection);
	setWrapping(true);
	//setFlow(QListView::TopToBottom);
	setLayoutMode(QListView::Batched);
	setResizeMode(QListView::Adjust);
	setAcceptDrops(true);
	setDragEnabled(true);
	setDragDropMode(QAbstractItemView::DragDrop);
	setDropIndicatorShown(true);
	connect(&model, SIGNAL(status(QString)), this, SIGNAL(status(QString)));
	if(!connect(this, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(applySubset(QModelIndex))))
		QMessageBox::information(0, "", "connection error");
	model.updateMode(M_ARTIST);
}

CollectionView::~CollectionView()
{

}

// start drag&drop block

/*void CollectionView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void CollectionView::dragLeaveEvent(QDragLeaveEvent *event)
{
}

void CollectionView::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void CollectionView::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
    	QList<QUrl> urls = event->mimeData()->urls();
    	foreach(QUrl url, urls) {
    		Database::Self().AddFile(url.toLocalFile());
    	}
        event->acceptProposedAction();
        updateModel(mode);
    } else {
        event->ignore();
    }
} 

void CollectionView::mousePressEvent(QMouseEvent *event)
{
	QModelIndex i1 = indexAt(event->pos());
	bool selected = false;
	foreach(QModelIndex ind, this->selectedIndexes()) {
		if(ind == i1) {
			selected = true;
			break;
		}
	}
	if(!selected || event->button() != Qt::LeftButton) {
		QListView::mousePressEvent(event);
		return;
	}

    QMimeData *mimeData = new QMimeData;
    QList<QUrl> list;
    foreach(QModelIndex ind, this->selectedIndexes()) {
		list += SelectByItem(ind);
    }
    QMessageBox::information(this, "", QString::number(list.count()));
    mimeData->setUrls(list);
    /*QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << list.count();
    foreach(QModelIndex ind, this->selectedIndexes()) {
        dataStream << ind.column();
        dataStream << model.data(ind, Qt::DisplayRole).toString();
    }
    mimeData->setData("application/x-playlist", itemData);*/
/*
    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setHotSpot(event->pos());
    //drag->setPixmap(pixmap);

    if (!(drag->exec(Qt::CopyAction) == Qt::CopyAction)) {
    	
    }
}
*/
// end drag&drop block

// select files
//QList<QUrl> CollectionView::SelectByItem(QModelIndex i)
//{
	//QList<QString> res;
	//switch(model.mode) {
	//case CollectionModel::M_ARTIST:
		//res = Database::Self().Songs(&model.data(i).toString(), 0, 0);
		//break;
	//case CollectionModel::M_ALBUM:
		//res = Database::Self().Songs(0, &model.data(i).toString(), 0);
		//break;
	//case CollectionModel::M_GENRE:
		//res = Database::Self().Songs(0, 0, &model.data(i).toString());
		//break;
	//}
	//QList<QUrl> urls;
	//foreach(QString s, res) {
		//urls.append(QUrl::fromLocalFile(s));
	//}
	////QUrl::fromLocalFile(model.data(ind, Qt::UserRole).toString()));
	//return urls;
//}

void CollectionView::artistMode()
{
	model.updateMode(M_ARTIST);
}

void CollectionView::albumMode()
{
	model.updateMode(M_ALBUM);
}

void CollectionView::genreMode()
{
	model.updateMode(M_GENRE);
}

void CollectionView::markMode()
{
	model.updateMode(M_MARK);
}

void CollectionView::addItem()
{
	switch(model.mode) {
	case M_ALBUM:
		Database::Self().AddAlbum(tr("New Album"));
		break;
	case M_ARTIST:
		Database::Self().AddArtist(tr("New Artist"));
		break;
	case M_GENRE:
		Database::Self().AddGenre(tr("New Genre"));
		break;
	case M_MARK:
		Database::Self().AddMark(tr("New Mark"));
		break;
	}
	model.update();
}

void CollectionView::removeItem()
{
    foreach(QModelIndex ind, this->selectedIndexes()) {
		switch(model.mode) {
		case M_ALBUM:
			Database::Self().RemoveAlbum(model.data(ind).toString());
			break;
		case M_ARTIST:
			Database::Self().RemoveArtist(model.data(ind).toString());
			break;
		case M_GENRE:
			Database::Self().RemoveGenre(model.data(ind).toString());
			break;
		case M_MARK:
			Database::Self().RemoveMark(model.data(ind).toString());
			break;
		}
    }
	//model.updateMode(model.mode);
    while(this->selectedIndexes().count()) model.removeRows(this->selectedIndexes().at(0).row(), 1);
    model.update();
}

void CollectionView::renameItem()
{
}

void CollectionView::filter(QString patt)
{
	model.searchPattern = patt;
	model.update();
}

void CollectionView::applySubset(QModelIndex ind)
{
	emit setVisibleSubsetWidgets(true);
	QString value = model.data(ind).toString();
	subsetLabel += QString(subsetLabel.length()?" - ":"") + value;
	emit setSubsetLabel(subsetLabel);
	switch(model.mode) {
	case M_ALBUM:
		Database::Self().subsetAlbum(value);
		break;
	case M_ARTIST:
		Database::Self().subsetArtist(value);
		break;
	case M_GENRE:
		Database::Self().subsetGenre(value);
		break;
	case M_MARK:
		Database::Self().subsetMark(value);
		break;
	}
	model.update();
}

void CollectionView::clearSubset()
{
	Database::Self().clearSubset();
	emit setVisibleSubsetWidgets(false);
	subsetLabel = "";
	emit setSubsetLabel(subsetLabel);
	model.update();
}