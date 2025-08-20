import React from 'react';
import { Icon } from './Icon';

type ActionItem = {
  label: string;
  icon?: string;
  onClick: () => void;
  isSeparator?: false;
};

type SeparatorItem = {
  isSeparator: true;
  label?: never;
  icon?: never;
  onClick?: never;
};

export type ContextAction = ActionItem | SeparatorItem;


interface ContextMenuProps {
  x: number;
  y: number;
  actions: ContextAction[];
  onClose: () => void;
}

export const ContextMenu: React.FC<ContextMenuProps> = ({ x, y, actions, onClose }) => {
  return (
    <>
      <div className="fixed inset-0 z-40" onClick={onClose} onContextMenu={(e) => { e.preventDefault(); onClose(); }} />
      <div
        className="fixed z-50 bg-slate-700 border border-slate-600 rounded-md shadow-lg py-1 w-56"
        style={{ top: y, left: x }}
      >
        <ul>
          {actions.map((action, index) =>
            action.isSeparator ? (
              <li key={`sep-${index}`} className="h-px bg-slate-600 my-1" />
            ) : (
              <li key={action.label}>
                <button
                  onClick={() => {
                    action.onClick();
                    onClose();
                  }}
                  className="w-full flex items-center gap-3 px-3 py-1.5 text-sm text-left text-slate-200 hover:bg-slate-600"
                  role="menuitem"
                >
                  {action.icon && <Icon name={action.icon} className="w-4 h-4 text-slate-400" />}
                  <span>{action.label}</span>
                </button>
              </li>
            )
          )}
        </ul>
      </div>
    </>
  );
};
