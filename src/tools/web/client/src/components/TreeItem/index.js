/**
 * @file
 *
 * @brief interactive tree view item to edit configurations of instances
 *
 * @copyright BSD License (see LICENSE.md or https://www.libelektra.org)
 */

import React, { Component } from 'react'

import ActionDelete from 'material-ui/svg-icons/action/delete'
import ActionBuild from 'material-ui/svg-icons/action/build'
import ContentAdd from 'material-ui/svg-icons/content/add'
import ContentCopy from 'material-ui/svg-icons/content/content-copy'
import ContentEdit from 'material-ui/svg-icons/editor/mode-edit'
import ContentPaste from 'material-ui/svg-icons/content/content-paste'
import { CopyToClipboard } from 'react-copy-to-clipboard'

import ActionButton from './ActionButton.jsx'
import SavedIcon from './SavedIcon.jsx'
import SimpleTextField from './fields/SimpleTextField.jsx'
import RadioButtons from './fields/RadioButtons.jsx'
import ToggleButton from './fields/ToggleButton.jsx'
import AddDialog from './dialogs/AddDialog.jsx'
import SettingsDialog from './dialogs/SettingsDialog.jsx'
import RemoveDialog from './dialogs/RemoveDialog.jsx'
import DuplicateDialog from './dialogs/DuplicateDialog.jsx'
import EditDialog from './dialogs/EditDialog.jsx'
import { parseEnum } from './utils'

export default class TreeItem extends Component {
  constructor (...args) {
    super(...args)
    this.state = {
      dialogs: {
        add: false,
        edit: false,
        settings: false,
        remove: false,
      },
      saved: false,
      savedTimeout: false,
    }
  }

  handleOpen = (dialog) => (e) => {
    e.stopPropagation()
    const { dialogs } = this.state
    this.setState({ dialogs: { ...dialogs, [dialog]: true } })
  }

  handleClose = (dialog) => () => {
    const { dialogs } = this.state
    this.setState({ dialogs: { ...dialogs, [dialog]: false } })
  }

  handleDelete = (path) => {
    const { instanceId, item, deleteKey, sendNotification } = this.props
    deleteKey(instanceId, path)
      .then(() => {
        if (Array.isArray(item.children) && item.children.length > 0) {
          return Promise.all(item.children.map(
            child => deleteKey(instanceId, child.path)
          ))
        }
      })
      .then(() => sendNotification('successfully deleted key: ' + path))
  }

  handleAdd = (path, addKeyName, addKeyValue) => {
    const { instanceId, setKey, sendNotification } = this.props
    const fullPath = path + '/' + addKeyName
    setKey(instanceId, fullPath, addKeyValue).then(() =>
      sendNotification('successfully created key: ' + fullPath)
    )
  }

  handleDuplicate = (from, to) => {
    const { instanceId, copyKey, sendNotification } = this.props
    copyKey(instanceId, from, to).then(() =>
      sendNotification('successfully duplicated key: ' + from + ' -> ' + to)
    )
  }

  handleEdit = (value) => {
    const { savedTimeout } = this.state
    const { instanceId, setKey, item } = this.props
    const { path } = item
    return setKey(instanceId, path, value)
      .then(() => {
        if (savedTimeout) clearTimeout(savedTimeout)
        this.setState({
          saved: true,
          savedTimeout: setTimeout(() => {
            this.setState({ saved: false })
          }, 1500),
        })
      })
  }

  renderSpecialValue = (id, { value, meta, onChange, label }) => {
    if (meta['check/type']) {
      if (meta['check/type'] === 'enum') {
        const valueFn = i => {
          return meta[`check/enum/#${i}`]
        }
        const options = parseEnum(valueFn)
        return (
            <RadioButtons id={id} value={value} meta={meta} options={options} onChange={onChange || this.handleEdit} />
        )
      }

      if (meta['check/type'] === 'boolean') {
        return (
            <ToggleButton label={label} id={id} value={value} meta={meta} onChange={onChange || this.handleEdit} />
        )
      }
    }
  }

  renderValue = (id, { value, meta, debounce = true, onChange, onKeyPress, onError, label }) => {
    const val = typeof value !== 'undefined' ? value : (meta && meta['default'])

    if (meta) {
      const special = this.renderSpecialValue(id, { value: val, meta, onChange, label })
      if (special) return special
    }

    // fallback
    return (
      <SimpleTextField debounce={debounce} label={label} id={id} value={val} meta={meta} onError={onError} onChange={onChange || this.handleEdit} onKeyPress={onKeyPress} />
    )
  }

  render () {
    const {
      data, item, instanceId, instanceVisibility,
      setMetaKey, deleteMetaKey, sendNotification,
    } = this.props

    const rootLevel = (item && item.path)
      ? !item.path.includes('/')
      : false

    const titleStyle = { marginTop: -3 }

    const meta = data && data.meta
    const isCheckbox = meta && meta['check/type'] && meta['check/type'] === 'boolean'
    const valueVisible = !rootLevel && data && !item.children
     // we return no value property if the key doesn't exist, otherwise we return an *empty* value
    const keyExists = rootLevel || (data && data.exists)

    return (
        <a style={{ display: 'flex', alignItems: 'center', opacity: keyExists ? 1 : 0.4 }}>
            {valueVisible
              ? (
                  <span style={{ display: 'flex', alignItems: 'center', height: 48 }}>
                      <b style={titleStyle}>{item.name + ': '}</b>
                      <span
                        style={{ marginLeft: 6 }}
                        onClick={(meta && meta.readonly === '1') ? (() => alert('This key is set to read-only and cannot be edited.')) : undefined}
                      >
                        {this.renderValue(item.path, data)}
                      </span>
                  </span>
                )
              : <b style={titleStyle}>{item.name}</b>
            }
            <span className="actions">
                <SavedIcon saved={this.state.saved} />
                {valueVisible &&
                  <CopyToClipboard text={(data && data.value) || ''} onCopy={() => sendNotification('Copied value of ' + item.path + ' to clipboard!')}>
                    <ActionButton icon={<ContentPaste />} tooltip="copy value" />
                  </CopyToClipboard>
                }
                <ActionButton icon={<ContentAdd />} onClick={this.handleOpen('add')} tooltip="create sub-key" />
                {!rootLevel && !valueVisible &&
                  <ActionButton icon={<ContentEdit />} onClick={this.handleOpen('edit')} tooltip="edit value" />
                }
                {!rootLevel &&
                  <ActionButton icon={<ContentCopy />} onClick={this.handleOpen('duplicate')} tooltip="duplicate key" />
                }
                {!rootLevel &&
                  <ActionButton icon={<ActionBuild />} onClick={this.handleOpen('settings')} size={13} tooltip="configure metadata" />
                }
                {!rootLevel &&
                  <ActionButton icon={<ActionDelete />} onClick={this.handleOpen('remove')} tooltip="delete key" />
                }
                <i>
                  {!isCheckbox && meta && meta.description}
                </i>
            </span>
            <AddDialog
              item={item}
              open={this.state.dialogs.add}
              onAdd={this.handleAdd}
              onClose={this.handleClose('add')}
              renderField={({ value, meta, debounce, onChange, onKeyPress, label, onError }) =>
                this.renderValue('addValueField', { value, meta, debounce, onChange, onKeyPress, label, onError })
              }
              setMetaByPath={(path, key, value) => setMetaKey(instanceId, path, key, value)}
            />
            <EditDialog
              field={this.renderValue(item.path, data || {})}
              item={item}
              value={data && data.value}
              open={this.state.dialogs.edit}
              onEdit={this.handleEdit}
              onClose={this.handleClose('edit')}
            />
            <DuplicateDialog
              item={item}
              open={this.state.dialogs.duplicate}
              onDuplicate={this.handleDuplicate}
              onClose={this.handleClose('duplicate')}
              pathExists={this.props.pathExists}
            />
            <SettingsDialog
              item={item}
              meta={data && data.meta}
              data={data && data.value}
              open={this.state.dialogs.settings}
              setMeta={(key, value) => setMetaKey(instanceId, item.path, key, value)}
              deleteMeta={key => deleteMetaKey(instanceId, item.path, key)}
              onClose={this.handleClose('settings')}
              onEdit={this.handleEdit}
              instanceVisibility={instanceVisibility}
            />
            <RemoveDialog
              item={item}
              open={this.state.dialogs.remove}
              onDelete={this.handleDelete}
              onClose={this.handleClose('remove')}
            />
        </a>
    )
  }
}
