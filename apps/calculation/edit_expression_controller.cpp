#include "edit_expression_controller.h"
#include "app.h"
#include "../apps_container.h"
#include <ion/display.h>
#include <poincare/preferences.h>
#include <assert.h>

using namespace Shared;
using namespace Poincare;

namespace Calculation {

EditExpressionController::ContentView::ContentView(Responder * parentResponder, TableView * subview, TextFieldDelegate * textFieldDelegate, LayoutFieldDelegate * layoutFieldDelegate) :
  View(),
  m_mainView(subview),
  m_expressionField(parentResponder, m_textBody, k_bufferLength, textFieldDelegate, layoutFieldDelegate)
{
  m_textBody[0] = 0;
}

View * EditExpressionController::ContentView::subviewAtIndex(int index) {
  assert(index >= 0 && index < numberOfSubviews());
  if (index == 0) {
    return m_mainView;
  }
  assert(index == 1);
  return &m_expressionField;
}

void EditExpressionController::ContentView::layoutSubviews() {
  KDCoordinate inputViewFrameHeight = m_expressionField.minimalSizeForOptimalDisplay().height();
  KDRect mainViewFrame(0, 0, bounds().width(), bounds().height() - inputViewFrameHeight);
  m_mainView->setFrame(mainViewFrame);
  KDRect inputViewFrame(0, bounds().height() - inputViewFrameHeight, bounds().width(), inputViewFrameHeight);
  m_expressionField.setFrame(inputViewFrame);
}

void EditExpressionController::ContentView::reload() {
  layoutSubviews();
  markRectAsDirty(bounds());
}

EditExpressionController::EditExpressionController(Responder * parentResponder, HistoryController * historyController, CalculationStore * calculationStore) :
  ViewController(parentResponder),
  m_historyController(historyController),
  m_calculationStore(calculationStore),
  m_contentView(this, (TableView *)m_historyController->view(), this, this),
  m_inputViewHeightIsMaximal(false)
{
  m_cacheBuffer[0] = 0;
}

View * EditExpressionController::view() {
  return &m_contentView;
}

void EditExpressionController::insertTextBody(const char * text) {
  ((ContentView *)view())->expressionField()->handleEventWithText(text, false, true);
}

bool EditExpressionController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::Up) {
    if (m_calculationStore->numberOfCalculations() > 0) {
      m_cacheBuffer[0] = 0;
      ((ContentView *)view())->expressionField()->setEditing(false, false);
      app()->setFirstResponder(m_historyController);
    }
    return true;
  }
  return false;
}

void EditExpressionController::didBecomeFirstResponder() {
  int lastRow = m_calculationStore->numberOfCalculations() > 0 ? m_calculationStore->numberOfCalculations()-1 : 0;
  m_historyController->scrollToCell(0, lastRow);
  ((ContentView *)view())->expressionField()->setEditing(true, false);
  app()->setFirstResponder(((ContentView *)view())->expressionField());
}

bool EditExpressionController::textFieldDidReceiveEvent(::TextField * textField, Ion::Events::Event event) {
  if (textField->isEditing() && textField->textFieldShouldFinishEditing(event) && textField->draftTextLength() == 0 && m_cacheBuffer[0] != 0) {
    return inputViewDidReceiveEvent(event);
  }
  return textFieldDelegateApp()->textFieldDidReceiveEvent(textField, event);
}

bool EditExpressionController::textFieldDidFinishEditing(::TextField * textField, const char * text, Ion::Events::Event event) {
  return inputViewDidFinishEditing(text, nullptr);
}

bool EditExpressionController::textFieldDidAbortEditing(::TextField * textField) {
  return inputViewDidAbortEditing(textField->text());
}

bool EditExpressionController::layoutFieldDidReceiveEvent(::LayoutField * layoutField, Ion::Events::Event event) {
  if (layoutField->isEditing() && layoutField->layoutFieldShouldFinishEditing(event) && !layoutField->hasText() && m_calculationStore->numberOfCalculations() > 0) {
    return inputViewDidReceiveEvent(event);
  }
  return expressionFieldDelegateApp()->layoutFieldDidReceiveEvent(layoutField, event);
}

bool EditExpressionController::layoutFieldDidFinishEditing(::LayoutField * layoutField, Layout layoutR, Ion::Events::Event event) {
  return inputViewDidFinishEditing(nullptr, layoutR);
}

bool EditExpressionController::layoutFieldDidAbortEditing(::LayoutField * layoutField) {
  return inputViewDidAbortEditing(nullptr);
}

void EditExpressionController::layoutFieldDidChangeSize(::LayoutField * layoutField) {
  /* Reload the view only if the ExpressionField height actually changes, i.e.
   * not if the height is already maximal and stays maximal. */
  if (view()) {
    bool newInputViewHeightIsMaximal = static_cast<ContentView *>(view())->expressionField()->heightIsMaximal();
    if (!m_inputViewHeightIsMaximal || !newInputViewHeightIsMaximal) {
      m_inputViewHeightIsMaximal = newInputViewHeightIsMaximal;
      reloadView();
    }
  }
}

TextFieldDelegateApp * EditExpressionController::textFieldDelegateApp() {
  return (App *)app();
}

ExpressionFieldDelegateApp * EditExpressionController::expressionFieldDelegateApp() {
  return (App *)app();
}

void EditExpressionController::reloadView() {
  ((ContentView *)view())->reload();
  m_historyController->reload();
  if (m_historyController->numberOfRows() > 0) {
    ((ContentView *)view())->mainView()->scrollToCell(0, m_historyController->numberOfRows()-1);
  }
}

bool EditExpressionController::inputViewDidReceiveEvent(Ion::Events::Event event) {
  App * calculationApp = (App *)app();
  /* The input text store in m_cacheBuffer might have beed correct the first
   * time but then be too long when replacing ans in another context */
  if (!calculationApp->textInputIsCorrect(m_cacheBuffer)) {
    return true;
  }
  m_calculationStore->push(m_cacheBuffer, calculationApp->localContext());
  m_historyController->reload();
  ((ContentView *)view())->mainView()->scrollToCell(0, m_historyController->numberOfRows()-1);
  return true;
}


bool EditExpressionController::inputViewDidFinishEditing(const char * text, Layout layoutR) {
  App * calculationApp = (App *)app();
  if (layoutR.isUninitialized()) {
    assert(text);
    strlcpy(m_cacheBuffer, text, Calculation::k_printedExpressionSize);
  } else {
    layoutR.serialize(m_cacheBuffer, Calculation::k_printedExpressionSize);
  }
  m_calculationStore->push(m_cacheBuffer, calculationApp->localContext());
  m_historyController->reload();
  ((ContentView *)view())->mainView()->scrollToCell(0, m_historyController->numberOfRows()-1);
  ((ContentView *)view())->expressionField()->setEditing(true, true);
  return true;
}

bool EditExpressionController::inputViewDidAbortEditing(const char * text) {
  if (text != nullptr) {
    ((ContentView *)view())->expressionField()->setEditing(true, true);
    ((ContentView *)view())->expressionField()->setText(text);
  }
  return false;
}

void EditExpressionController::viewDidDisappear() {
  m_historyController->viewDidDisappear();
}

}
