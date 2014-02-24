#include "emacskeysplugin.h"
#include "emacskeysconstants.h"
#include "emacskeysstate.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <utils/qtcassert.h>

#include <texteditor/basetexteditor.h>

#include <QAction>
#include <QPlainTextEdit>
#include <QApplication>
#include <QClipboard>
#include <QScrollBar>

#include <QtPlugin>

using namespace EmacsKeys::Internal;

//---------------------------------------------------------------------------
// EmacsKeysPlugin
//---------------------------------------------------------------------------

EmacsKeysPlugin::EmacsKeysPlugin()
{
	// Create your members
}

EmacsKeysPlugin::~EmacsKeysPlugin()
{
	// Unregister objects from the plugin manager's object pool
	// Delete members
}

bool EmacsKeysPlugin::initialize(const QStringList &arguments, QString *errorString)
{
	// Register objects in the plugin manager's object pool
	// Load settings
	// Add actions to menus
	// Connect to other plugins' signals
	// In the initialize function, a plugin can be sure that the plugins it
	// depends on have initialized their members.

	Q_UNUSED(arguments)
	Q_UNUSED(errorString)

	connect(Core::EditorManager::instance(),
		SIGNAL(editorAboutToClose(Core::IEditor*)),
		this,
		SLOT(editorAboutToClose(Core::IEditor*)));
	connect(Core::EditorManager::instance(),
		SIGNAL(currentEditorChanged(Core::IEditor*)),
		this,
		SLOT(currentEditorChanged(Core::IEditor*)));

	registerAction(Constants::DELETE_CHARACTER,
		SLOT(deleteCharacter()), tr("Delete Character"));
	registerAction(Constants::KILL_WORD,
		SLOT(killWord()), tr("Kill Word"));
	registerAction(Constants::KILL_LINE,
		SLOT(killLine()), tr("Kill Line"));
	registerAction(Constants::INSERT_LINE_AND_INDENT,
		SLOT(insertLineAndIndent()), tr("Insert New Line and Indent"));

	registerAction(Constants::GOTO_FILE_START,
		SLOT(gotoFileStart()), tr("Go to File Start"));
	registerAction(Constants::GOTO_FILE_END,
		SLOT(gotoFileEnd()), tr("Go to File End"));
	registerAction(Constants::GOTO_LINE_START,
		SLOT(gotoLineStart()), tr("Go to Line Start"));
	registerAction(Constants::GOTO_LINE_END,
		SLOT(gotoLineEnd()), tr("Go to Line End"));
	registerAction(Constants::GOTO_NEXT_LINE,
		SLOT(gotoNextLine()), tr("Go to Next Line"));
	registerAction(Constants::GOTO_PREVIOUS_LINE,
		SLOT(gotoPreviousLine()), tr("Go to Previous Line"));
	registerAction(Constants::GOTO_NEXT_CHARACTER,
		SLOT(gotoNextCharacter()), tr("Go to Next Character"));
	registerAction(Constants::GOTO_PREVIOUS_CHARACTER,
		SLOT(gotoPreviousCharacter()), tr("Go to Previous Character"));
	registerAction(Constants::GOTO_NEXT_WORD,
		SLOT(gotoNextWord()), tr("Go to Next Word"));
	registerAction(Constants::GOTO_PREVIOUS_WORD,
		SLOT(gotoPreviousWord()), tr("Go to Previous Word"));

	registerAction(Constants::MARK,
		SLOT(mark()), tr("Mark"));
	registerAction(Constants::EXCHANGE_CURSOR_AND_MARK,
		SLOT(exchangeCursorAndMark()), tr("Exchange Cursor and Mark"));
	registerAction(Constants::COPY,
		SLOT(copy()), tr("Copy"));
	registerAction(Constants::CUT,
		SLOT(cut()), tr("Cut"));
	registerAction(Constants::YANK,
		SLOT(yank()), tr("Yank"));

	registerAction(Constants::SCROLL_HALF_DOWN,
		SLOT(scrollHalfDown()), tr("Scroll Half Screen Down"));
	registerAction(Constants::SCROLL_HALF_UP,
		SLOT(scrollHalfUp()), tr("Scroll Half Screen Up"));
	return true;
}

void EmacsKeysPlugin::extensionsInitialized()
{
	// Retrieve objects from the plugin manager's object pool
	// In the extensionsInitialized function, a plugin can be sure that all
	// plugins that depend on it are completely initialized.
}

ExtensionSystem::IPlugin::ShutdownFlag EmacsKeysPlugin::aboutToShutdown()
{
	// Save settings
	// Disconnect from signals that are not needed during shutdown
	// Hide UI (if you add UI that is not in the main window directly)
	return SynchronousShutdown;
}

void EmacsKeysPlugin::editorAboutToClose(Core::IEditor *editor)
{
	QPlainTextEdit *w = qobject_cast<QPlainTextEdit*>(editor->widget());
	if (!w)
		return;

	if (m_stateMap.contains(w)) {
		delete m_stateMap[w];
		m_stateMap.remove(w);
	}
}

void EmacsKeysPlugin::currentEditorChanged(Core::IEditor *editor)
{
	if (!editor) {
		m_currentEditorWidget = 0;
		return;
	}
	m_currentEditorWidget = qobject_cast<QPlainTextEdit*>(editor->widget());
	if (!m_currentEditorWidget)
		return;

	if (!m_stateMap.contains(m_currentEditorWidget)) {
		m_stateMap[m_currentEditorWidget] = new EmacsKeysState(m_currentEditorWidget);
	}
	m_currentState = m_stateMap[m_currentEditorWidget];
	m_currentBaseTextEditorWidget =
		qobject_cast<TextEditor::BaseTextEditorWidget*>(editor->widget());
}

void EmacsKeysPlugin::gotoFileStart()         { genericGoto(QTextCursor::Start); }
void EmacsKeysPlugin::gotoFileEnd()           { genericGoto(QTextCursor::End); }
void EmacsKeysPlugin::gotoLineStart()         { genericGoto(QTextCursor::StartOfLine); }
void EmacsKeysPlugin::gotoLineEnd()           { genericGoto(QTextCursor::EndOfLine); }
void EmacsKeysPlugin::gotoNextLine()          { genericGoto(QTextCursor::Down, false); }
void EmacsKeysPlugin::gotoPreviousLine()      { genericGoto(QTextCursor::Up, false); }
void EmacsKeysPlugin::gotoNextCharacter()     { genericGoto(QTextCursor::Right); }
void EmacsKeysPlugin::gotoPreviousCharacter() { genericGoto(QTextCursor::Left); }
void EmacsKeysPlugin::gotoNextWord()          { genericGoto(QTextCursor::NextWord); }
void EmacsKeysPlugin::gotoPreviousWord()      { genericGoto(QTextCursor::PreviousWord); }

void EmacsKeysPlugin::mark()
{
	if (!m_currentEditorWidget)
		return;

	m_currentState->beginOwnAction();
	QTextCursor cursor = m_currentEditorWidget->textCursor();
	if (m_currentState->mark() == cursor.position()) {
		m_currentState->setMark(-1);
	} else {
		cursor.clearSelection();
		m_currentState->setMark(cursor.position());
		m_currentEditorWidget->setTextCursor(cursor);
	}
	m_currentState->endOwnAction(EKA_OTHER);
}

void EmacsKeysPlugin::exchangeCursorAndMark()
{
	if (!m_currentEditorWidget)
		return;

	QTextCursor cursor = m_currentEditorWidget->textCursor();
	if (m_currentState->mark() == -1 || m_currentState->mark() == cursor.position())
		return;

	m_currentState->beginOwnAction();
	int position = cursor.position();
	cursor.clearSelection();
	cursor.setPosition(m_currentState->mark(), QTextCursor::KeepAnchor);
	m_currentState->setMark(position);
	m_currentEditorWidget->setTextCursor(cursor);
	m_currentState->endOwnAction(EKA_OTHER);
}

void EmacsKeysPlugin::copy()
{
	if (!m_currentEditorWidget)
		return;

	m_currentState->beginOwnAction();
	QTextCursor cursor = m_currentEditorWidget->textCursor();
	QApplication::clipboard()->setText(cursor.selectedText());
	cursor.clearSelection();
	m_currentEditorWidget->setTextCursor(cursor);
	m_currentState->setMark(-1);
	m_currentState->endOwnAction(EKA_OTHER);
}

void EmacsKeysPlugin::cut()
{
	if (!m_currentEditorWidget)
		return;

	m_currentState->beginOwnAction();
	QTextCursor cursor = m_currentEditorWidget->textCursor();
	QApplication::clipboard()->setText(cursor.selectedText());
	cursor.removeSelectedText();
	m_currentState->setMark(-1);
	m_currentState->endOwnAction(EKA_OTHER);
}

void EmacsKeysPlugin::yank()
{
	if (!m_currentEditorWidget)
		return;

	m_currentState->beginOwnAction();
	m_currentEditorWidget->paste();
	m_currentState->setMark(-1);
	m_currentState->endOwnAction(EKA_OTHER);
}

void EmacsKeysPlugin::scrollHalfDown() { genericVScroll(1); }
void EmacsKeysPlugin::scrollHalfUp() { genericVScroll(-1); }

void EmacsKeysPlugin::deleteCharacter()
{
	if (!m_currentEditorWidget)
		return;
	m_currentState->beginOwnAction();
	m_currentEditorWidget->textCursor().deleteChar();
	m_currentState->endOwnAction(EKA_OTHER);
}

void EmacsKeysPlugin::killWord()
{
	if (!m_currentEditorWidget)
		return;
	m_currentState->beginOwnAction();
	QTextCursor cursor = m_currentEditorWidget->textCursor();
	cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
	if (m_currentState->lastAction() == EKA_KILL_WORD) {
		QApplication::clipboard()->setText(
			QApplication::clipboard()->text() + cursor.selectedText());
	} else {
		QApplication::clipboard()->setText(cursor.selectedText());
	}
	cursor.removeSelectedText();
	m_currentState->endOwnAction(EKA_KILL_WORD);
}

void EmacsKeysPlugin::killLine()
{
	if (!m_currentEditorWidget)
		return;

	m_currentState->beginOwnAction();
	QTextCursor cursor = m_currentEditorWidget->textCursor();
	int position = cursor.position();
	cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
	if (cursor.position() == position) {
		// empty line
		cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
	}
	if (m_currentState->lastAction() == EKA_KILL_LINE) {
		QApplication::clipboard()->setText(
			QApplication::clipboard()->text() + cursor.selectedText());
	} else {
		QApplication::clipboard()->setText(cursor.selectedText());
	}
	cursor.removeSelectedText();
	m_currentState->endOwnAction(EKA_KILL_LINE);
}

void EmacsKeysPlugin::insertLineAndIndent()
{
	if (!m_currentEditorWidget)
		return;

	m_currentState->beginOwnAction();
	QTextCursor cursor = m_currentEditorWidget->textCursor();
	cursor.beginEditBlock();
	cursor.insertBlock();
	if (m_currentBaseTextEditorWidget != 0)
		m_currentBaseTextEditorWidget->baseTextDocument()->autoIndent(cursor);
	cursor.endEditBlock();
	m_currentEditorWidget->setTextCursor(cursor);
	m_currentState->endOwnAction(EKA_OTHER);
}

QAction *EmacsKeysPlugin::registerAction(const Core::Id &id, const char *slot,
	const QString &title)
{
	QAction *result = new QAction(title, this);
	Core::ActionManager::registerAction(result, id,
		Core::Context(Core::Constants::C_GLOBAL), true);

	connect(result, SIGNAL(triggered(bool)), this, slot);
	return result;
}

void EmacsKeysPlugin::genericGoto(QTextCursor::MoveOperation op, bool abortAssist)
{
	if (!m_currentEditorWidget)
		return;
	m_currentState->beginOwnAction();
	QTextCursor cursor = m_currentEditorWidget->textCursor();
	cursor.movePosition(op, m_currentState->mark() != -1 ?
		QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
	m_currentEditorWidget->setTextCursor(cursor);
	if (abortAssist && m_currentBaseTextEditorWidget != 0)
		m_currentBaseTextEditorWidget->abortAssist();
	m_currentState->endOwnAction(EKA_OTHER);
}

void EmacsKeysPlugin::genericVScroll(int direction)
{
	if (!m_currentEditorWidget)
		return;

	m_currentState->beginOwnAction();
	QScrollBar *verticalScrollBar = m_currentEditorWidget->verticalScrollBar();
	const int value = verticalScrollBar->value();
	const int halfPageStep = verticalScrollBar->pageStep() / 2;
	const int newValue = value + (direction > 0 ? halfPageStep : -halfPageStep);
	verticalScrollBar->setValue(newValue);

	// adjust cursor if it's out of screen
	const QRect viewportRect = m_currentEditorWidget->viewport()->rect();
	const QTextCursor::MoveMode mode =
		m_currentState->mark() != -1 ?
		QTextCursor::KeepAnchor :
		QTextCursor::MoveAnchor ;
	const QTextCursor::MoveOperation op =
		m_currentEditorWidget->cursorRect().y() < 0 ?
		QTextCursor::Down :
		QTextCursor::Up ;

	QTextCursor cursor = m_currentEditorWidget->textCursor();
	while (!m_currentEditorWidget->cursorRect(cursor).intersects(viewportRect)) {
		const int previousPosition = cursor.position();
		cursor.movePosition(op, mode);
		if (previousPosition == cursor.position())
			break;
	}
	m_currentEditorWidget->setTextCursor(cursor);
	m_currentState->endOwnAction(EKA_OTHER);
}

Q_EXPORT_PLUGIN2(EmacsKeys, EmacsKeysPlugin)

